import tkinter as tk
from tkinter import scrolledtext, filedialog, messagebox
import ttkbootstrap as ttk
from ttkbootstrap.constants import *
from ttkbootstrap.tooltip import ToolTip
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
import os
import sys

# ==========================================
# 🌍 多语言字典
# ==========================================
STRINGS = {
    "title": ["Moil的XUnity大模型翻译GUI", "Moil'sXUnity LLM Translator GUI"],
    "grp_api": ["API 配置", "API Configuration"],
    "lbl_addr": ["API 地址:", "API Address:"],
    "lbl_key": ["API 密钥:", "API Key:"],
    "lbl_model": ["模型名称:", "Model Name:"],
    "btn_fetch": ["获取", "Fetch"],
    "lbl_port": ["端口:", "Port:"],
    "lbl_thread": ["线程:", "Threads:"],
    "lbl_temp": ["温度:", "Temp:"],
    "lbl_ctx": ["上下文:", "Context:"],
    "lbl_sys": ["系统提示:", "System Prompt:"],
    "lbl_pre": ["前置文本:", "Pre-Prompt:"],
    "lbl_glossary": ["术语表:", "Glossary:"],
    "chk_glossary": ["启用自进化 (实验性)", "Enable Self-Evolution (Exp)"],
    "btn_file": ["...", "..."],
    "grp_log": ["运行日志", "Runtime Logs"],
    
    "btn_start": ["启动服务", "Start Server"],
    "btn_stop": ["停止服务", "Stop Server"],
    "btn_test": ["测试配置", "Test Config"],
    "btn_load": ["读取配置", "Load Config"],
    "btn_save": ["保存配置", "Save Config"],
    "btn_export": ["导出日志", "Export Log"],
    "btn_theme": ["切换亮色", "Light Mode"],
    "btn_lang": ["English", "中文"],
    "btn_clear": ["清空日志", "Clear Log"],
    
    "log_start": ["服务已启动，端口：", "Server started at port: "],
    "log_stop": ["服务已停止", "Server stopped"],
    "log_req": ["收到请求: ", "Request received: "],
    "log_trans": ["翻译结果: ", "Translation: "],
    "log_err_key": ["错误: API密钥无效", "Error: Invalid API Key"],
    "log_glossary_match": ["📖 匹配术语: ", "📖 Term Matched: "],
    "log_saved": ["配置已保存至: ", "Config saved to: "],
    "log_loaded": ["配置已加载自: ", "Config loaded from: "],

    "tip_port": ["本地监听端口。\n请确保 XUnity 配置文件 Endpoint 设置为 http://localhost:端口号", "Local Listening Port\nEnsure XUnity Endpoint is set to http://localhost:port"],
    "tip_thread": ["并发线程数 \n建议值: 取决于你电脑的线程数\n注意: 一定程度上可以加快翻译工作，过多会导致系统卡顿", "Concurrent Threads\nRecommended: Depends on your CPU\nNote: Can speed up translation to some extent, too many may cause system lag"],
    "tip_temp": ["采样温度 (0.0-2.0) \n0.0-0.3: 严谨\n0.7-1.0: 标准\n>1.0: 随机/创造性", "Sampling Temperature (0.0-2.0)\n0.0-0.3: Strict\n0.7-1.0: Standard\n>1.0: Creative/Random"],
    "tip_ctx": ["上下文记忆轮数 \n携带的历史对话轮数。\n注意：上下文越多，消耗 Token 越多。", "Context Memory\nNumber of history turns to carry.\nNote: More context consumes more tokens."],
    "tip_glossary": ["选择 _Substitutions.txt 文件 \nLLM 将自动参考并补充该文件。", "Select _Substitutions.txt file\nLLM will reference and append to it."]
}

class GlossaryManager:
    def __init__(self):
        self.terms = {}
        self.file_path = ""

    def load(self, path):
        self.terms = {}
        self.file_path = path
        if not path or not os.path.exists(path): return
        try:
            with open(path, 'r', encoding='utf-8') as f:
                for line in f:
                    if '=' in line:
                        parts = line.split('=', 1)
                        k, v = parts[0].strip(), parts[1].strip()
                        if k and v: self.terms[k] = v
        except: pass

    def get_context_prompt(self, text):
        if not self.terms: return ""
        found = [f"{k} = {v}" for k, v in self.terms.items() if k in text]
        if not found: return ""
        return "\n\n【已知术语/Known Terms】:\n" + "\n".join(found) + "\n"

    def add_term(self, key, value):
        if not self.file_path or key in self.terms: return
        self.terms[key] = value
        try:
            with open(self.file_path, 'a', encoding='utf-8') as f:
                f.write(f"{key}={value}\n")
        except: pass

glossary_mgr = GlossaryManager()

class ConfigManager:
    def __init__(self, filename='config.ini'):
        self.filename = filename
        self.config = configparser.ConfigParser()
        self.config.read(filename, encoding='utf-8')

    def save_config(self, settings):
        if not self.config.has_section('Settings'): self.config.add_section('Settings')
        for key, value in settings.items(): self.config.set('Settings', key, str(value))
        with open(self.filename, 'w', encoding='utf-8') as f: self.config.write(f)

    def load_config(self):
        default_prompt = "你是一个游戏翻译模型，可以流畅通顺地将任意的游戏文本翻译成简体中文..."
        return {
            'api_address': self.config.get('Settings', 'api_address', fallback='https://api.openai.com/v1'),
            'api_key': self.config.get('Settings', 'api_key', fallback=''),
            'model_name': self.config.get('Settings', 'model_name', fallback='gpt-3.5-turbo'),
            'port': self.config.get('Settings', 'port', fallback='6800'),
            'max_threads': self.config.getint('Settings', 'max_threads', fallback=20),
            'system_prompt': self.config.get('Settings', 'system_prompt', fallback=default_prompt),
            'pre_prompt': self.config.get('Settings', 'pre_prompt', fallback='将下面的文本翻译成简体中文：'),
            'context_num': self.config.getint('Settings', 'context_num', fallback=5),
            'temperature': self.config.getfloat('Settings', 'temperature', fallback=1.0),
            'language': self.config.getint('Settings', 'language', fallback=0),
            'theme': self.config.get('Settings', 'theme', fallback='darkly'),
            'enable_glossary': self.config.getboolean('Settings', 'enable_glossary', fallback=False),
            'glossary_path': self.config.get('Settings', 'glossary_path', fallback='')
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
        api_keys = [k.strip() for k in raw_key.split(',') if k.strip()]
        if not api_keys: return None
        with TranslationHandler._key_lock:
            if TranslationHandler._round_robin_index >= len(api_keys): TranslationHandler._round_robin_index = 0
            key = api_keys[TranslationHandler._round_robin_index]
            TranslationHandler._round_robin_index = (TranslationHandler._round_robin_index + 1) % len(api_keys)
        return key

    def get_client_id(self):
        return hashlib.md5(self.client_address[0].encode()).hexdigest()[:8]

    def do_GET(self):
        config = self.get_config()
        lang = config.get('language', 0)
        api_key = self.get_next_api_key(config)
        
        if not api_key:
            self.log_queue.put(STRINGS["log_err_key"][lang])
            self.send_response(500); self.end_headers(); return

        try:
            parsed = urllib.parse.urlparse(self.path)
            if parsed.path != "/": self.send_response(404); self.end_headers(); return
            
            text = urllib.parse.parse_qs(parsed.query).get('text', [''])[0].strip()
            if not text: self.send_response(200); self.end_headers(); return

            self.log_queue.put(f"{STRINGS['log_req'][lang]}{text}")

            sys_prompt = config['system_prompt']
            if config['enable_glossary']:
                ctx = glossary_mgr.get_context_prompt(text)
                if ctx: 
                    sys_prompt += ctx
                    self.log_queue.put(f"{STRINGS['log_glossary_match'][lang]}Yes")
                sys_prompt += "\nInstruction: Extract NEW proper nouns to <tm>Original=Translated</tm> tags."

            client = OpenAI(base_url=config['api_address'], api_key=api_key)
            
            cid = self.get_client_id()
            with self._lock:
                if cid not in self._contexts: self._contexts[cid] = deque(maxlen=config['context_num'])
                if self._contexts[cid].maxlen != config['context_num']: 
                    self._contexts[cid] = deque(self._contexts[cid], maxlen=config['context_num'])

            msgs = [{"role": "system", "content": sys_prompt}]
            with self._lock:
                for u, a in self._contexts[cid]:
                    msgs.append({"role": "user", "content": u})
                    msgs.append({"role": "assistant", "content": a})
            
            curr_u = f"{config['pre_prompt']}{text}"
            msgs.append({"role": "user", "content": curr_u})

            resp = client.chat.completions.create(
                model=config['model_name'], messages=msgs, temperature=config['temperature']
            )
            raw_trans = resp.choices[0].message.content
            
            trans = raw_trans
            if config['enable_glossary']:
                for m in re.finditer(r"<tm>(.*?)</tm>", raw_trans):
                    term = m.group(1).strip()
                    if '=' in term:
                        k, v = term.split('=', 1)
                        if k.strip() in text:
                            glossary_mgr.add_term(k.strip(), v.strip())
                trans = re.sub(r"<[^>]*>", "", raw_trans).strip()
            
            trans = re.sub(r'<think>.*?</think>', '', trans, flags=re.DOTALL).strip()

            with self._lock: self._contexts[cid].append((curr_u, trans))

            self.send_response(200)
            self.send_header('Content-type', 'text/plain; charset=utf-8')
            self.end_headers()
            self.wfile.write(trans.encode('utf-8'))
            self.log_queue.put(f"{STRINGS['log_trans'][lang]}{trans}")

        except Exception as e:
            self.log_queue.put(f"Error: {str(e)}")
            self.send_response(500); self.end_headers()

    @classmethod
    def create_handler(cls, get_config_func, log_queue):
        return lambda *args, **kwargs: cls(get_config_func, log_queue, *args, **kwargs)

class TranslationApp:
    def __init__(self, master):
        self.master = master
        self.master.geometry("720x800") 
        
        
        self.current_config_file = 'config.ini'
        self.config = ConfigManager(self.current_config_file).load_config()
        self.log_queue = queue.Queue()
        self.server = None
        self.tooltips = {}
        
        # ✨ 提示语常量
        self.GLOSSARY_PLACEHOLDER = "_Substitutions.txt Path..."

        self.current_lang = self.config.get('language', 0)
        self.current_theme = self.config.get('theme', 'darkly')
        self.style = ttk.Style(theme=self.current_theme)
        
        if self.config['enable_glossary']:
            glossary_mgr.load(self.config['glossary_path'])

        self.create_widgets()
        self._load_config_to_ui()
        self.update_ui_text()
        self.update_log()
        
        self.master.attributes('-alpha', 0.0)
        self.fade_in()
        master.protocol("WM_DELETE_WINDOW", self.on_closing)

    def fade_in(self):
        alpha = self.master.attributes('-alpha')
        if alpha < 1.0:
            self.master.attributes('-alpha', min(alpha + 0.05, 1.0))
            self.master.after(20, self.fade_in)

    def smooth_switch(self, callback):
        for i in range(10, 0, -2):
            self.master.attributes('-alpha', i/10); self.master.update(); time.sleep(0.01)
        callback()
        for i in range(0, 11, 2):
            self.master.attributes('-alpha', i/10); self.master.update(); time.sleep(0.01)

    def on_closing(self):
        self.save_config()
        self.stop_server()
        self.master.destroy()

    def create_widgets(self):
        main_frame = ttk.Frame(self.master)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=15, pady=15)

        # === API Config ===
        self.grp_api = ttk.Labelframe(main_frame, text="API Config")
        self.grp_api.grid(row=0, column=0, sticky="nsew", pady=5)
        self.grp_api.columnconfigure(1, weight=1)

        # Row 0-2 (Address, Key, Model)
        self.lbl_addr = ttk.Label(self.grp_api, anchor="e")
        self.lbl_addr.grid(row=0, column=0, sticky="e", padx=5, pady=5)
        self.api_address = ttk.Entry(self.grp_api)
        self.api_address.grid(row=0, column=1, sticky="ew", padx=5, pady=5)

        self.lbl_key = ttk.Label(self.grp_api, anchor="e")
        self.lbl_key.grid(row=1, column=0, sticky="e", padx=5, pady=5)
        self.api_key = ttk.Entry(self.grp_api, show="*")
        self.api_key.grid(row=1, column=1, sticky="ew", padx=5, pady=5)

        self.lbl_model = ttk.Label(self.grp_api, anchor="e")
        self.lbl_model.grid(row=2, column=0, sticky="e", padx=5, pady=5)
        model_frame = ttk.Frame(self.grp_api)
        model_frame.grid(row=2, column=1, sticky="ew", padx=5, pady=5)
        model_frame.columnconfigure(0, weight=1)
        self.model_name = ttk.Combobox(model_frame)
        self.model_name.grid(row=0, column=0, sticky="ew", padx=(0,5))
        self.btn_fetch = ttk.Button(model_frame, text="Fetch", command=self.fetch_model_list, bootstyle="outline")
        self.btn_fetch.grid(row=0, column=1)

        # Row 3: Params
        param_frame = ttk.Frame(self.grp_api)
        param_frame.grid(row=3, column=0, columnspan=2, sticky="ew", padx=5, pady=10)
        
        self.lbl_port = ttk.Label(param_frame, text="Port:")
        self.lbl_port.pack(side="left", padx=(0,5))
        self.port = ttk.Entry(param_frame, width=6, justify="center")
        self.port.pack(side="left", padx=(0,15))
        self.tooltips['port'] = ToolTip(self.port, text="")

        self.lbl_thread = ttk.Label(param_frame, text="Threads:")
        self.lbl_thread.pack(side="left", padx=(0,5))
        self.threads = ttk.Spinbox(param_frame, from_=1, to=100, width=4, justify="center")
        self.threads.pack(side="left", padx=(0,15))
        self.tooltips['thread'] = ToolTip(self.threads, text="")

        self.lbl_temp = ttk.Label(param_frame, text="Temp:")
        self.lbl_temp.pack(side="left", padx=(0,5))
        self.temperature = ttk.Spinbox(param_frame, from_=0.0, to=2.0, increment=0.1, width=4, justify="center")
        self.temperature.pack(side="left", padx=(0,15))
        self.tooltips['temp'] = ToolTip(self.temperature, text="")

        self.lbl_ctx = ttk.Label(param_frame, text="Ctx:")
        self.lbl_ctx.pack(side="left", padx=(0,5))
        self.context_num = ttk.Spinbox(param_frame, from_=0, to=20, width=4, justify="center")
        self.context_num.pack(side="left")
        self.tooltips['ctx'] = ToolTip(self.context_num, text="")

        # Row 4-5 (Prompts)
        self.lbl_sys = ttk.Label(self.grp_api, anchor="ne")
        self.lbl_sys.grid(row=4, column=0, sticky="ne", padx=5, pady=5)
        self.system_prompt = scrolledtext.ScrolledText(self.grp_api, height=5, wrap=tk.WORD)
        self.system_prompt.grid(row=4, column=1, sticky="ew", padx=5, pady=5)

        self.lbl_pre = ttk.Label(self.grp_api, anchor="e")
        self.lbl_pre.grid(row=5, column=0, sticky="e", padx=5, pady=5)
        self.pre_prompt = ttk.Entry(self.grp_api)
        self.pre_prompt.grid(row=5, column=1, sticky="ew", padx=5, pady=5)

        # Row 6: Glossary (带 Placeholder 逻辑)
        self.lbl_glossary = ttk.Label(self.grp_api, anchor="e")
        self.lbl_glossary.grid(row=6, column=0, sticky="e", padx=5, pady=5)
        
        glossary_frame = ttk.Frame(self.grp_api)
        glossary_frame.grid(row=6, column=1, sticky="ew", padx=5, pady=5)
        
        self.chk_glossary_var = tk.BooleanVar()
        self.chk_glossary = ttk.Checkbutton(glossary_frame, variable=self.chk_glossary_var)
        self.chk_glossary.pack(side="left", padx=(0,5))
        self.tooltips['glossary'] = ToolTip(self.chk_glossary, text="")
        
        self.glossary_path = ttk.Entry(glossary_frame)
        self.glossary_path.pack(side="left", fill="x", expand=True, padx=(0,5))
        
        # ✨ 绑定 Focus 事件实现 Placeholder
        self.glossary_path.bind("<FocusIn>", self._on_glossary_focus_in)
        self.glossary_path.bind("<FocusOut>", self._on_glossary_focus_out)
        
        self.btn_file = ttk.Button(glossary_frame, text="...", width=3, command=self.select_glossary_file, bootstyle="outline")
        self.btn_file.pack(side="left")

        # === Buttons ===
        btn_frame = ttk.Frame(main_frame)
        btn_frame.grid(row=1, column=0, pady=10, sticky="ew")
        
        f_left = ttk.Frame(btn_frame)
        f_left.pack(side="left")
        self.btn_start = ttk.Button(f_left, command=self.start_server, bootstyle="success")
        self.btn_start.pack(side="left", padx=2)
        self.btn_stop = ttk.Button(f_left, command=self.stop_server, bootstyle="danger", state="disabled")
        self.btn_stop.pack(side="left", padx=2)
        self.btn_test = ttk.Button(f_left, command=self.test_config, bootstyle="info")
        self.btn_test.pack(side="left", padx=2)
        self.btn_load = ttk.Button(f_left, command=self.load_config_dialog, bootstyle="secondary-outline")
        self.btn_load.pack(side="left", padx=2)
        self.btn_save = ttk.Button(f_left, command=self.save_config_dialog, bootstyle="secondary-outline")
        self.btn_save.pack(side="left", padx=2)
        self.btn_export = ttk.Button(f_left, command=self.export_log, bootstyle="secondary-outline")
        self.btn_export.pack(side="left", padx=2)

        f_right = ttk.Frame(btn_frame)
        f_right.pack(side="right")
        self.btn_lang = ttk.Button(f_right, command=self.toggle_language, bootstyle="warning-outline")
        self.btn_lang.pack(side="left", padx=2)
        self.btn_theme = ttk.Button(f_right, command=self.toggle_theme, bootstyle="dark-outline")
        self.btn_theme.pack(side="left", padx=2)

        # === Logs ===
        self.grp_log = ttk.Labelframe(main_frame, text="Logs")
        self.grp_log.grid(row=2, column=0, sticky="nsew", pady=5)
        self.log_area = scrolledtext.ScrolledText(self.grp_log, height=10, state="disabled")
        self.log_area.pack(fill=tk.BOTH, expand=True)
        
        self.context_menu = tk.Menu(self.log_area, tearoff=0)
        self.context_menu.add_command(label="Clear", command=lambda: self.log_area.delete(1.0, tk.END))
        self.log_area.bind("<Button-3>", self.show_context_menu)

        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(2, weight=1)

    # ✨ Placeholder 逻辑
    def _on_glossary_focus_in(self, event):
        if self.glossary_path.get() == self.GLOSSARY_PLACEHOLDER:
            self.glossary_path.delete(0, tk.END)
            self.glossary_path.configure(foreground=self.get_normal_color())

    def _on_glossary_focus_out(self, event):
        if not self.glossary_path.get():
            self.glossary_path.insert(0, self.GLOSSARY_PLACEHOLDER)
            self.glossary_path.configure(foreground='gray')

    def get_normal_color(self):
        return 'white' if 'dark' in self.current_theme else 'black'

    def show_context_menu(self, event):
        self.context_menu.entryconfigure(0, label=STRINGS["btn_clear"][self.current_lang])
        self.context_menu.post(event.x_root, event.y_root)

    def update_ui_text(self):
        l = self.current_lang
        self.master.title(STRINGS["title"][l])
        self.grp_api.config(text=STRINGS["grp_api"][l])
        self.lbl_addr.config(text=STRINGS["lbl_addr"][l])
        self.lbl_key.config(text=STRINGS["lbl_key"][l])
        self.lbl_model.config(text=STRINGS["lbl_model"][l])
        self.btn_fetch.config(text=STRINGS["btn_fetch"][l])
        self.lbl_port.config(text=STRINGS["lbl_port"][l])
        self.lbl_thread.config(text=STRINGS["lbl_thread"][l])
        self.lbl_temp.config(text=STRINGS["lbl_temp"][l])
        self.lbl_ctx.config(text=STRINGS["lbl_ctx"][l])
        self.lbl_sys.config(text=STRINGS["lbl_sys"][l])
        self.lbl_pre.config(text=STRINGS["lbl_pre"][l])
        self.lbl_glossary.config(text=STRINGS["lbl_glossary"][l])
        self.chk_glossary.config(text=STRINGS["chk_glossary"][l])
        self.grp_log.config(text=STRINGS["grp_log"][l])
        
        self.btn_start.config(text=STRINGS["btn_start"][l])
        self.btn_stop.config(text=STRINGS["btn_stop"][l])
        self.btn_test.config(text=STRINGS["btn_test"][l])
        self.btn_load.config(text=STRINGS["btn_load"][l])
        self.btn_save.config(text=STRINGS["btn_save"][l])
        self.btn_export.config(text=STRINGS["btn_export"][l])
        self.btn_lang.config(text=STRINGS["btn_lang"][l])
        
        theme_idx = 0 if self.current_theme == 'darkly' else 1
        self.btn_theme.config(text=STRINGS["btn_theme"][theme_idx])
        self.btn_theme.configure(bootstyle="light-outline" if self.current_theme == 'darkly' else "dark-outline")

        self.tooltips['port'].text = STRINGS["tip_port"][l]
        self.tooltips['thread'].text = STRINGS["tip_thread"][l]
        self.tooltips['temp'].text = STRINGS["tip_temp"][l]
        self.tooltips['ctx'].text = STRINGS["tip_ctx"][l]
        self.tooltips['glossary'].text = STRINGS["tip_glossary"][l]

    def toggle_language(self):
        self.smooth_switch(lambda: setattr(self, 'current_lang', 1 if self.current_lang == 0 else 0) or self.update_ui_text())

    def toggle_theme(self):
        def logic():
            self.current_theme = 'flatly' if self.current_theme == 'darkly' else 'darkly'
            self.style.theme_use(self.current_theme)
            self.update_ui_text()
            # 切换主题时，如果当前显示的是 Placeholder，需要更新颜色
            if self.glossary_path.get() == self.GLOSSARY_PLACEHOLDER:
                self.glossary_path.configure(foreground='gray')
            else:
                self.glossary_path.configure(foreground=self.get_normal_color())
        self.smooth_switch(logic)

    def select_glossary_file(self):
        path = filedialog.askopenfilename(filetypes=[("Text Files", "*.txt"), ("All Files", "*.*")])
        if path:
            self.glossary_path.delete(0, tk.END)
            self.glossary_path.insert(0, path)
            self.glossary_path.configure(foreground=self.get_normal_color()) # 恢复颜色

    def get_config(self):
        try: ctx = int(self.context_num.get())
        except: ctx = 5
        try: temp = float(self.temperature.get())
        except: temp = 1.0
        try: th = int(self.threads.get())
        except: th = 20
        
        # ✨ 获取路径时，如果是 Placeholder 则视为空
        g_path = self.glossary_path.get()
        if g_path == self.GLOSSARY_PLACEHOLDER:
            g_path = ""

        return {
            'api_address': self.api_address.get(),
            'api_key': self.api_key.get(),
            'model_name': self.model_name.get(),
            'port': self.port.get(),
            'max_threads': th,
            'pre_prompt': self.pre_prompt.get(),
            'system_prompt': self.system_prompt.get('1.0', 'end-1c'),
            'context_num': ctx,
            'temperature': temp,
            'language': self.current_lang,
            'theme': self.current_theme,
            'enable_glossary': self.chk_glossary_var.get(),
            'glossary_path': g_path
        }

    def _load_config_to_ui(self):
        self.api_address.delete(0, tk.END); self.api_address.insert(0, self.config['api_address'])
        self.api_key.delete(0, tk.END); self.api_key.insert(0, self.config['api_key'])
        self.model_name.set(self.config['model_name'])
        self.port.delete(0, tk.END); self.port.insert(0, self.config['port'])
        self.threads.delete(0, tk.END); self.threads.insert(0, self.config['max_threads'])
        self.temperature.delete(0, tk.END); self.temperature.insert(0, str(self.config['temperature']))
        self.pre_prompt.delete(0, tk.END); self.pre_prompt.insert(0, self.config['pre_prompt'])
        self.system_prompt.delete('1.0', tk.END); self.system_prompt.insert('1.0', self.config['system_prompt'])
        self.context_num.set(self.config['context_num'])
        self.chk_glossary_var.set(self.config['enable_glossary'])
        
        # ✨ 加载路径逻辑
        self.glossary_path.delete(0, tk.END)
        path = self.config['glossary_path']
        if path:
            self.glossary_path.insert(0, path)
            self.glossary_path.configure(foreground=self.get_normal_color())
        else:
            self.glossary_path.insert(0, self.GLOSSARY_PLACEHOLDER)
            self.glossary_path.configure(foreground='gray')

    def start_server(self):
        cfg = self.get_config()
        if cfg['enable_glossary']: glossary_mgr.load(cfg['glossary_path'])
        try:
            self.server = ThreadingHTTPServer(('localhost', int(cfg['port'])), TranslationHandler.create_handler(lambda: self.get_config(), self.log_queue))
            self.server_thread = threading.Thread(target=self.server.serve_forever); self.server_thread.daemon = True; self.server_thread.start()
            self.toggle_controls(True)
            self.log_queue.put(f"{STRINGS['log_start'][self.current_lang]}{cfg['port']}")
        except Exception as e: self.log_queue.put(f"Start Error: {str(e)}")

    def stop_server(self):
        if self.server:
            self.server.shutdown(); self.server.server_close(); self.server = None
            self.toggle_controls(False)
            self.log_queue.put(STRINGS['log_stop'][self.current_lang])

    def fetch_model_list(self):
        cfg = self.get_config()
        try:
            url = cfg['api_address'].rstrip("/") + "/models"
            headers = {"Authorization": f"Bearer {cfg['api_key']}"}
            r = requests.get(url, headers=headers, timeout=10)
            if r.status_code == 200:
                models = [item['id'] for item in r.json().get('data', [])]
                self.model_name['values'] = models
                if models: self.model_name.set(models[0])
                self.log_queue.put("Fetch Success")
            else: self.log_queue.put(f"Fetch Error: {r.status_code}")
        except Exception as e: self.log_queue.put(f"Fetch Error: {e}")

    def test_config(self):
        cfg = self.get_config()
        keys = [k.strip() for k in cfg['api_key'].split(',') if k.strip()]
        if not keys: self.log_queue.put(STRINGS['log_err_key'][self.current_lang]); return
        for k in keys: threading.Thread(target=self._run_test, args=(k, cfg)).start()

    def _run_test(self, key, cfg):
        try:
            client = OpenAI(base_url=cfg['api_address'], api_key=key)
            client.chat.completions.create(model=cfg['model_name'], messages=[{"role": "user", "content": "Hi"}], timeout=10)
            self.log_queue.put(f"✅ Key ({key[:8]}...): OK")
        except Exception as e: self.log_queue.put(f"❌ Key ({key[:8]}...): Fail - {e}")

    def save_config(self): ConfigManager(self.current_config_file).save_config(self.get_config())
    def save_config_dialog(self):
        path = filedialog.asksaveasfilename(defaultextension=".ini", initialfile="config.ini")
        if path: self.current_config_file = path; self.save_config(); self.log_queue.put(f"{STRINGS['log_saved'][self.current_lang]}{path}")
    def load_config_dialog(self):
        path = filedialog.askopenfilename(defaultextension=".ini")
        if path:
            self.current_config_file = path; self.config = ConfigManager(path).load_config(); self._load_config_to_ui()
            self.current_lang = self.config['language']; self.current_theme = self.config['theme']
            self.style.theme_use(self.current_theme); self.update_ui_text()
            self.log_queue.put(f"{STRINGS['log_loaded'][self.current_lang]}{path}")
    def export_log(self):
        try:
            with open("run_log.txt", "w", encoding="utf-8") as f: f.write(self.log_area.get("1.0", "end-1c"))
            self.log_queue.put("Log Exported")
        except: pass

    def toggle_controls(self, running):
        state = "disabled" if running else "normal"
        self.stop_btn.config(state="normal" if running else "disabled")
        self.btn_start.config(state="disabled" if running else "normal")
        for w in [self.api_address, self.api_key, self.model_name, self.port, self.threads,
                  self.temperature, self.context_num, self.chk_glossary, self.glossary_path]:
            w.config(state=state)

    def update_log(self):
        while not self.log_queue.empty():
            msg = self.log_queue.get()
            self.log_area.config(state="normal"); self.log_area.insert(tk.END, msg + "\n"); self.log_area.see(tk.END); self.log_area.config(state="disabled")
        self.master.after(100, self.update_log)

if __name__ == "__main__":
    root = ttk.Window(themename="darkly")

    try:
        root.iconbitmap("moil.ico")
    except:
        pass
    app = TranslationApp(root)
    root.mainloop()
