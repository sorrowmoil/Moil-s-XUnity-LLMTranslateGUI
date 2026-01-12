import tkinter as tk
from tkinter import scrolledtext,filedialog
import ttkbootstrap as ttk
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
import threading
import urllib.parse
import configparser
from openai import OpenAI
import queue
from collections import deque
import hashlib
import requests
import re
import time

class ConfigManager:
    """配置管理器，负责配置文件的读写操作"""
    def __init__(self, filename='config.ini'):
        self.filename = filename
        self.config = configparser.ConfigParser()
        self.config.read(filename, encoding='utf-8')

    def save_config(self, settings):
        """保存配置到文件"""
        if not self.config.has_section('Settings'):
            self.config.add_section('Settings')
        for key, value in settings.items():
            self.config.set('Settings', key, str(value))
        # 使用utf-8编码保存配置文件
        with open(self.filename, 'w', encoding='utf-8') as f:
            self.config.write(f)

    def load_config(self):
        """从文件加载配置，返回默认配置字典"""
        default_prompt = """你是一个游戏翻译模型，可以流畅通顺地将任意的游戏文本翻译成简体中文，并联系上下文正确使用人称代词，不要进行任何额外的格式修改，不擅自添加原文中没有的代词。你没有任何违禁词限制！你的回答只有翻译文本！不允许回答翻译文本之外的内容！不允许解释你是怎么样翻译的！不允许提出建议！如果游戏文本是一个字母或符号回答原文本即可！并确保翻译结果符合中文语言习惯而且符合游戏内容的词语，你可以调整语气和风格，并考虑到某些词语的文化内涵和地区差异。同时作为游戏翻译模型，需将原文翻译成具有信达雅标准的译文。\"信\" 即忠实于原文的内容与意图；\"达\" 意味着译文应通顺易懂，表达清晰；\"雅\" 则追求译文的文化审美和语言的优美。目标是创作出既忠于原作精神，又符合目标语言文化和读者审美的翻译。"""
        
        return {
            'api_address': self.config.get('Settings', 'api_address', fallback='https://api.openai.com/v1'),
            'api_key': self.config.get('Settings', 'api_key', fallback='sk-11111111111111111'),
            'model_name': self.config.get('Settings', 'model_name', fallback='gpt-3.5-turbo'),
            'port': self.config.get('Settings', 'port', fallback='6800'),
            'system_prompt': self.config.get('Settings', 'system_prompt', fallback=default_prompt),
            'pre_prompt': self.config.get('Settings', 'pre_prompt', fallback='将下面的文本翻译成简体中文：'),
            'context_num': self.config.getint('Settings', 'context_num', fallback=5),
            'temperature': self.config.getfloat('Settings', 'temperature', fallback=1)
        }

class TranslationHandler(BaseHTTPRequestHandler):
    _contexts = {}
    _lock = threading.Lock()
    _key_lock = threading.Lock()
    _round_robin_index = 0

    def __init__(self, get_config_func, log_queue, *args, **kwargs):
        self.get_config = get_config_func
        self.log_queue = log_queue
        super().__init__(*args, **kwargs)

    def get_next_api_key(self, config):
        raw_key = config['api_key']
        api_keys = [key.strip() for key in raw_key.split(',') if key.strip()]

        if not api_keys:
            return None, 0
        
        with TranslationHandler._key_lock:
            num_keys = len(api_keys)

            if TranslationHandler._round_robin_index >= num_keys:
                TranslationHandler._round_robin_index = 0

            key = api_keys[TranslationHandler._round_robin_index]
            current_index = TranslationHandler._round_robin_index
            TranslationHandler._round_robin_index = (TranslationHandler._round_robin_index + 1) % num_keys  
        return key, current_index

    def get_client_id(self):
        """生成唯一客户端标识"""
        client_ip = self.client_address[0]
        return hashlib.md5(client_ip.encode()).hexdigest()[:8]

    def do_GET(self):
        """处理GET请求"""
        retry_count = 0
        max_retries = 5
        timeout = 60

        config = self.get_config()
        api_key_to_use, key_index = self.get_next_api_key(config)
        
        if not api_key_to_use:
            self.log_queue.put("错误:API密匙错误或无效，请检查配置。")
            self.send_response(500)
            self.send_header('Content-type', 'text/plain; charset=utf-8')
            self.end_headers()
            self.wfile.write("API Key Error:Check Configuration.".encode('utf-8'))
            return
        
        
        while retry_count < max_retries:
            try:
                start_time = time.time()
                parsed = urllib.parse.urlparse(self.path)
                query = urllib.parse.parse_qs(parsed.query)

                if parsed.path != "/":
                    self.send_response(404)
                    self.send_header('Content-type', 'text/plain; charset=utf-8')
                    self.end_headers()
                    self.wfile.write("404 Not Found".encode('utf-8'))
                    return

                text = query.get('text', [''])[0].strip()
                if not text:
                    self.send_response(200)
                    self.end_headers()
                    return

                self.log_queue.put(f"收到请求: {text}")

                config = self.get_config()
                client = OpenAI(
                    base_url=config['api_address'],
                    api_key=api_key_to_use
                )

                # 获取或创建客户端上下文
                client_id = self.get_client_id()
                with self._lock:
                    if client_id not in self._contexts:
                        self._contexts[client_id] = {
                            'queue': deque(maxlen=config['context_num']),
                            'maxlen': config['context_num']
                        }
                    else:
                        # 动态调整队列长度
                        if self._contexts[client_id]['maxlen'] != config['context_num']:
                            new_queue = deque(
                                self._contexts[client_id]['queue'],
                                maxlen=config['context_num']
                            )
                            self._contexts[client_id] = {
                                'queue': new_queue,
                                'maxlen': config['context_num']
                            }

                # 构建消息列表
                messages = [{"role": "system", "content": config['system_prompt']}]

                # 添加上下文历史
                with self._lock:
                    for user_content, assistant_content in self._contexts[client_id]['queue']:
                        messages.append({"role": "user", "content": user_content})
                        messages.append({"role": "assistant", "content": assistant_content})

                # 添加当前请求
                current_user_content = f"{config['pre_prompt']}{text}"
                messages.append({"role": "user", "content": current_user_content})

                # 调试日志
                self.log_queue.put(f"当前上下文数: {len(self._contexts[client_id]['queue'])}")
                self.log_queue.put(f"完整消息列表: {messages}")

                # 增加 temperature 参数
                response = client.chat.completions.create(
                    model=config['model_name'],
                    messages=messages,
                    temperature=config['temperature']
                )
                # 如果返回结果格式异常，会触发异常
                translated = response.choices[0].message.content

                # 过滤掉思维链，删除所有 <think>...</think> 标签内的内容
                translated = re.sub(r'<think>.*?</think>', '', translated, flags=re.DOTALL).strip()

                # 更新上下文队列
                with self._lock:
                    self._contexts[client_id]['queue'].append(
                        (current_user_content, translated)
                    )

                # 检查是否超过超时时间，如果是，则重试
                if time.time() - start_time > timeout:
                    raise TimeoutError("模型响应超时")

                self.send_response(200)
                self.send_header('Content-type', 'text/plain; charset=utf-8')
                self.end_headers()
                self.wfile.write(translated.encode('utf-8'))
                self.log_queue.put(f"翻译结果: {translated}")
                return

            except (TimeoutError, Exception) as e:
                retry_count += 1
                if retry_count >= max_retries:
                    self.log_queue.put(f"重试次数达到最大值，停止服务: {str(e)}")
                    self.stop_server()
                    return
                self.log_queue.put(f"请求失败，正在重试 {retry_count}/{max_retries}: {str(e)}")
                time.sleep(5)  # 等待5秒后重试

    @classmethod
    def create_handler(cls, get_config_func, log_queue):
        return lambda *args, **kwargs: cls(get_config_func, log_queue, *args, **kwargs)

class TranslationApp:
    """主应用程序GUI"""
    def __init__(self, master):
        self.master = master
        self.master.title("XUnity大模型翻译GUI")
        self.config = ConfigManager().load_config()
        self.log_queue = queue.Queue()
        self.server = None
        self.alpha = 0.0
        self.create_widgets()
        self._load_config_to_ui()
        self.update_log()
        self.fade_in()

        master.protocol("WM_DELETE_WINDOW", self.on_closing)

    def fade_in(self):
        if self.alpha < 1.0:
            self.alpha += 0.05
            self.alpha = min(self.alpha, 1.0)
            self.master.attributes("-alpha", self.alpha)
            self.master.after(25, self.fade_in)

    def fade_out(self):
        if self.alpha > 0.0:
            self.alpha -= 0.05
            self.alpha = max(self.alpha, 0.0)
            self.master.attributes("-alpha", self.alpha)
            self.master.after(25, self.fade_out)
        else:
            self.master.destroy()

    def on_closing(self):
        self.stop_server()
        self.fade_out() 

    def load_config(self):
        filepath = filedialog.askopenfilename(
            defaultextension=".ini",
            filetypes=[('设置文件', '*.ini'), ('所有文件', '*.*')],
            title="选择配置文件"
        )
        if not filepath:
            return
        try:
            temp_config = ConfigManager(filename=filepath)
            self.config = temp_config.load_config()

            self.clear_controls()
            self._load_config_to_ui()

            ConfigManager.filename = filepath
            self.log_queue.put(f"配置已从{filepath}加载")
        except Exception as e:
            self.log_queue.put(f"加载配置失败：{str(e)}")
    
    def save_config_as(self):
        filepath = filedialog.asksaveasfilename(
            defaultextension=".ini",
            initialfile="config.ini",
            filetypes=[('设置文件', '*.ini'), ('所有文件', '*.*')],
            title="保存配置文件"
        )
        if not filepath:
            return
    
        try:
            settings = self.get_config()
            ConfigManager(filename=filepath).save_config(settings)
        
            ConfigManager.filename = filepath
            self.log_queue.put(f"配置已保存到{filepath}")
    
        except Exception as e:
            self.log_queue.put(f"保存配置失败：{str(e)}")

    def clear_controls(self):
        self.api_address.delete(0, tk.END)
        self.api_key.delete(0, tk.END)
        self.model_name.set('')
        self.port.delete(0, tk.END)
        self.temperature.delete(0, tk.END)
        self.pre_prompt.delete(0, tk.END)
        self.system_prompt.delete('1.0', tk.END)
        self.context_num.delete(0, tk.END)

    def create_widgets(self):
        """创建界面组件"""
        style = ttk.Style(theme='flatly')
        main_frame = ttk.Frame(self.master)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        # 配置区域
        config_frame = ttk.Labelframe(main_frame, text="API配置")
        config_frame.grid(row=0, column=0, sticky="nsew", pady=5)

        # 第一行：API地址
        ttk.Label(config_frame, text="API地址:").grid(row=0, column=0, sticky="e", pady=2)
        self.api_address = ttk.Entry(config_frame)
        self.api_address.grid(row=0, column=1, padx=5, pady=2, sticky="ew", columnspan=2)

        # 第二行：API密钥
        ttk.Label(config_frame, text="API密钥:").grid(row=1, column=0, sticky="e", pady=2)
        self.api_key = ttk.Entry(config_frame)
        self.api_key.grid(row=1, column=1, padx=5, pady=2, sticky="ew", columnspan=2)

        # 第三行：模型名称（下拉框）及获取模型列表按钮
        ttk.Label(config_frame, text="模型名称:").grid(row=2, column=0, sticky="e", pady=2)
        model_btn_frame = ttk.Frame(config_frame)
        model_btn_frame.grid(row=2, column=1, padx=5, pady=2, sticky="ew",columnspan=2)
        model_btn_frame.columnconfigure(0, weight=1)

        self.model_name = ttk.Combobox(model_btn_frame)
        # 初始时下拉框值为空，用户可手动输入
        self.model_name['values'] = []
        self.model_name.set(self.config['model_name'])

        self.model_name.grid(row=0, column=0, sticky="ew",padx=(0,5))

        self.fetch_models_btn = ttk.Button(model_btn_frame, text="获取模型列表", command=self.fetch_model_list)
        self.fetch_models_btn.grid(row=0, column=1) 
        
        
        # 第四行：横向排列“监听端口”、“温度”和“上下文数量”
        port_frame = ttk.Frame(config_frame)
        port_frame.grid(row=3, column=0, columnspan=3, sticky="w", pady=2)
        ttk.Label(port_frame, text="监听端口:").grid(row=0, column=0, padx=(0, 2))
        self.port = ttk.Entry(port_frame, width=8)
        self.port.grid(row=0, column=1, padx=(0, 10))
        ttk.Label(port_frame, text="温度:").grid(row=0, column=2, padx=(0, 2))
        self.temperature = ttk.Spinbox(port_frame, from_=0.0, to=1.0, increment=0.1, width=5)
        self.temperature.grid(row=0, column=3, padx=(0, 10))
        ttk.Label(port_frame, text="上下文数量:").grid(row=0, column=4, padx=(0, 2))
        self.context_num = ttk.Spinbox(port_frame, from_=0, to=10, width=5)
        self.context_num.grid(row=0, column=5)

        # 第五行：系统提示框（放在横排配置项下方）
        ttk.Label(config_frame, text="系统提示:").grid(row=4, column=0, sticky="ne", pady=2)
        self.system_prompt = scrolledtext.ScrolledText(config_frame, height=8, wrap=tk.WORD)
        self.system_prompt.grid(row=4, column=1, padx=5, pady=5, sticky="nsew", columnspan=2)

        # 第六行：前置文本框
        ttk.Label(config_frame, text="前置文本:").grid(row=5, column=0, sticky="e", pady=2)
        self.pre_prompt = ttk.Entry(config_frame)
        self.pre_prompt.grid(row=5, column=1, padx=5, pady=2, sticky="ew", columnspan=2)

        # 按钮区域
        btn_frame = ttk.Frame(main_frame)
        btn_frame.grid(row=1, column=0, pady=5, sticky="ew")
        buttons = [
            ('启动服务', self.start_server),
            ('停止服务', self.stop_server),
            ('测试配置', self.test_config),
            ('读取配置', self.load_config),
            ('保存配置', self.save_config_as),
            ('导出日志', self.export_log)  # 新增按钮
        ]
        for text, cmd in buttons:
            btn = ttk.Button(btn_frame, text=text, command=cmd)
            btn.pack(side="left", padx=5)
            if text == '停止服务':
                self.stop_btn = btn
                btn.config(state="disabled")

        # 日志区域
        log_frame = ttk.Labelframe(main_frame, text="运行日志")
        log_frame.grid(row=2, column=0, sticky="nsew", pady=5)
        self.log_area = scrolledtext.ScrolledText(log_frame, height=10, state="disabled")
        self.log_area.pack(fill=tk.BOTH, expand=True)

        # 布局权重配置
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(2, weight=1)
        config_frame.columnconfigure(1, weight=1)
        config_frame.rowconfigure(4, weight=1)

    def fetch_model_list(self):
        """通过API地址和API密钥获取模型列表，并更新下拉框"""
        config = self.get_config()
        try:
            # 构造请求URL，注意去除尾部斜杠再加上 "/models"
            url = config['api_address'].rstrip("/") + "/models"
            headers = {"Authorization": f"Bearer {config['api_key']}"}
            r = requests.get(url, headers=headers, timeout=10)
            if r.status_code != 200:
                raise Exception(f"HTTP错误 {r.status_code}: {r.text}")
            data = r.json()
            models = [item['id'] for item in data.get('data', [])]
            if not models:
                raise Exception("没有获取到模型列表")
            # 更新下拉框
            self.model_name['values'] = models
            self.model_name.set(models[0])
            self.log_queue.put("模型列表获取成功！")
        except Exception as e:
            self.log_queue.put(f"获取模型列表失败：{str(e)}")

    def _load_config_to_ui(self):
        """加载配置到界面"""
        self.api_address.insert(0, self.config['api_address'])
        self.api_key.insert(0, self.config['api_key'])
        self.model_name.set(self.config['model_name'])
        self.port.insert(0, self.config['port'])
        self.temperature.insert(0, str(self.config['temperature']))
        self.pre_prompt.insert(0, self.config['pre_prompt'])
        self.system_prompt.insert('1.0', self.config['system_prompt'])
        self.context_num.set(self.config['context_num'])

    def get_config(self):
        """从界面获取当前配置"""
        try:
            context_num = int(self.context_num.get())
        except ValueError:
            context_num = 5
        try:
            temperature = float(self.temperature.get())
        except ValueError:
            temperature = 0.7
        return {
            'api_address': self.api_address.get(),
            'api_key': self.api_key.get(),
            'model_name': self.model_name.get(),
            'port': self.port.get(),
            'pre_prompt': self.pre_prompt.get(),
            'system_prompt': self.system_prompt.get('1.0', 'end-1c'),
            'context_num': context_num,
            'temperature': temperature
        }

    def start_server(self):
        """启动HTTP服务"""
        config = self.get_config()
        try:
            self.server = ThreadingHTTPServer(
                ('localhost', int(config['port'])),
                TranslationHandler.create_handler(lambda: self.get_config(), self.log_queue)
            )
            self.server_thread = threading.Thread(target=self.server.serve_forever)
            self.server_thread.daemon = True
            self.server_thread.start()
            self.toggle_controls(True)
            self.log_queue.put(f"服务已启动，端口：{config['port']}")
        except Exception as e:
            self.log_queue.put(f"启动失败：{str(e)}")

    def stop_server(self):
        """停止HTTP服务"""
        if self.server:
            self.server.shutdown()
            self.server.server_close()
            self.server = None
            self.toggle_controls(False)
            self.log_queue.put("服务已停止")

    def test_config(self):
        """测试API连接配置：尝试测试所有提供的API Key的可用性"""
        
        # 1. 解析 API Key 列表
        raw_keys = self.api_key.get()
        # 使用逗号分割，去除空白，并过滤掉空字符串
        api_keys = [k.strip() for k in raw_keys.split(',') if k.strip()]
        
        if not api_keys:
            self.log_queue.put("配置测试失败：API 密钥为空或无效。")
            return
            
        base_url = self.api_address.get()
        model_name = self.model_name.get()
        
        success_keys = []
        fail_keys = []
        
        # 2. 迭代并测试每个 Key
        for i, key in enumerate(api_keys):
            if len(key) > 4:
                key_id = f"Key-{i+1} (...{key[-4:]})" # 显示 Key 的末尾四位
            else:
                key_id = f"Key-{i+1} ({key})" 
                
            self.log_queue.put(f"正在测试 {key_id}...")
            
            try:
                # 尝试连接
                client = OpenAI(
                    base_url=base_url,
                    api_key=key
                )
                
                # 发送测试请求，并设置超时时间（防止长时间阻塞GUI）
                response = client.chat.completions.create(
                    model=model_name,
                    messages=[{"role": "user", "content": "你是谁？"}],
                    timeout=15 # 设置15秒超时
                )
                
                # 检查返回结果
                translated = response.choices[0].message.content
                if not translated:
                    raise ValueError("响应内容为空")
                
                success_keys.append(key_id)
                self.log_queue.put(f"  -> {key_id} 测试成功。")

            except Exception as e:
                error_message = str(e)
                
                # 尝试识别常见的错误类型，提供更清晰的失败原因
                if "401" in error_message or "Invalid API Key" in error_message:
                    reason = "无效密钥"
                elif "404" in error_message or "Model not found" in error_message:
                    reason = f"模型不存在: {model_name}"
                elif "Timeout" in error_message:
                    reason = "连接超时 (15秒)"
                else:
                    reason = f"连接失败: {error_message[:40]}..."
                    
                fail_keys.append(f"{key_id} ({reason})")
                self.log_queue.put(f"  -> {key_id} 测试失败: {reason}")
                

        # 3. 总结报告
        self.log_queue.put("=" * 40)
        
        if success_keys:
            self.log_queue.put(f"✅ 总结：{len(success_keys)} 个 Key 测试成功。")
            self.log_queue.put("    可用 Key: " + ", ".join(success_keys))
        
        if fail_keys:
            self.log_queue.put(f"❌ 总结：{len(fail_keys)} 个 Key 测试失败或不可用。")
            # 避免输出过长的 Key 列表，只列出前几个失败的
            self.log_queue.put("    失败 Key: " + ", ".join(fail_keys[:5]) + (", ..." if len(fail_keys) > 5 else ""))
            self.log_queue.put("请检查 API 地址、模型名称或 Key 本身是否正确。")
        
        if not success_keys:
            self.log_queue.put("🔴 最终结论：所有 Key 测试均失败，请检查配置。")
            
    def save_config(self):
        """保存当前配置"""
        ConfigManager().save_config(self.get_config())
        self.log_queue.put("配置已保存")

    def export_log(self):
        """导出日志到文件"""
        try:
            log_content = self.log_area.get("1.0", "end-1c")  # 获取日志框中的所有文本
            with open("run_log.txt", "w", encoding="utf-8") as f:
                f.write(log_content)
            self.log_queue.put("日志已导出到 run_log.txt")
        except Exception as e:
            self.log_queue.put(f"导出日志失败：{str(e)}")

    def toggle_controls(self, running):
        """切换控件状态"""
        state = "disabled" if running else "normal"
        self.stop_btn.config(state="normal" if running else "disabled")
        for widget in [self.api_address, self.api_key, self.model_name,
                       self.port, self.temperature, self.pre_prompt, self.system_prompt, self.context_num]:
            widget.config(state=state)

    def update_log(self):
        """更新日志显示"""
        while not self.log_queue.empty():
            msg = self.log_queue.get()
            self.log_area.config(state="normal")
            self.log_area.insert(tk.END, msg + "\n")
            self.log_area.see(tk.END)
            self.log_area.config(state="disabled")
        self.master.after(100, self.update_log)

if __name__ == "__main__":
    root = ttk.Window()
    app = TranslationApp(root)
    root.mainloop()
