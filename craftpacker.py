import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import requests
import threading
import os
import time
from pathlib import Path
import re
import webbrowser
import sv_ttk

# --- Modrinth API Constants ---
MODRINTH_API_BASE = "https://api.modrinth.com/v2"

# --- Project URLs ---
GITHUB_URL = "https://github.com/helloworldx64/CraftPacker"
PAYPAL_URL = "https://www.paypal.com/donate/?business=4UZWFGSW6C478&no_recurring=0&item_name=Donate+to+helloworldx64&currency_code=USD"

# --- Rate Limiter Class ---
class RateLimiter:
    """A thread-safe rate limiter to prevent exceeding API request limits."""
    def __init__(self, calls_per_minute):
        self.min_interval = 60.0 / calls_per_minute
        self.lock = threading.Lock()
        self.last_call_time = 0

    def wait(self):
        with self.lock:
            elapsed = time.time() - self.last_call_time
            wait_time = self.min_interval - elapsed
            if wait_time > 0:
                time.sleep(wait_time)
            self.last_call_time = time.time()

# --- Main Application ---
class CraftPackerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("CraftPacker")
        self.root.geometry("950x650")

        self.rate_limiter = RateLimiter(calls_per_minute=280)
        self.results = {}
        self.downloading_set = set()
        self.total_searched = 0
        self.resolving_popup = None

        self.setup_ui()
        sv_ttk.set_theme("light")

    def setup_ui(self):
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.pack(fill="both", expand=True)

        settings_frame = ttk.LabelFrame(main_frame, text="Settings", padding="10")
        settings_frame.pack(fill="x", pady=(0, 10))
        settings_frame.columnconfigure(1, weight=1)

        ttk.Label(settings_frame, text="MC Version:").grid(row=0, column=0, padx=5, sticky="w")
        self.version_entry = ttk.Entry(settings_frame, width=15)
        self.version_entry.insert(0, "1.20.1")
        self.version_entry.grid(row=0, column=1, padx=5, sticky="ew")

        ttk.Label(settings_frame, text="Loader:").grid(row=0, column=2, padx=5, sticky="w")
        self.loader_var = tk.StringVar(value="fabric")
        loader_box = ttk.Combobox(settings_frame, textvariable=self.loader_var, values=["fabric", "forge", "neoforge", "quilt"], width=12, state="readonly")
        loader_box.grid(row=0, column=3, padx=5, sticky="w")

        self.dark_mode_var = tk.BooleanVar()
        dark_mode_check = ttk.Checkbutton(settings_frame, text="Dark Mode", variable=self.dark_mode_var, command=self.toggle_theme)
        dark_mode_check.grid(row=0, column=4, padx=10, sticky="e")
        settings_frame.columnconfigure(4, weight=1)

        ttk.Label(settings_frame, text="Download To:").grid(row=1, column=0, padx=5, sticky="w")
        self.dir_var = tk.StringVar(value=str(Path.home() / "CraftPacker_Downloads"))
        dir_entry = ttk.Entry(settings_frame, textvariable=self.dir_var)
        dir_entry.grid(row=1, column=1, columnspan=3, padx=5, pady=5, sticky="ew")
        ttk.Button(settings_frame, text="Browse...", command=self.browse_dir).grid(row=1, column=4, padx=5, sticky="w")

        content_frame = ttk.Frame(main_frame)
        content_frame.pack(fill="both", expand=True, pady=5)
        
        input_frame = ttk.LabelFrame(content_frame, text="Mod List")
        input_frame.pack(side="left", fill="both", expand=False, padx=(0, 10))
        
        self.text_input = tk.Text(input_frame, height=8, width=30)
        self.text_input.pack(fill="both", expand=True, padx=5, pady=5)
        
        import_button = ttk.Button(input_frame, text="Import from Folder...", command=self.import_mods_from_folder)
        import_button.pack(fill="x", padx=5, pady=(0, 5))

        results_area = ttk.Frame(content_frame)
        results_area.pack(side="left", fill="both", expand=True)

        found_frame = ttk.LabelFrame(results_area, text="Available Mods")
        found_frame.pack(side="left", fill="both", expand=True, padx=(0, 5))
        
        not_found_frame = ttk.LabelFrame(results_area, text="Not Found")
        not_found_frame.pack(side="left", fill="both", expand=True, padx=(5, 0))

        columns = ("official_name", "status", "progress")
        self.tree = ttk.Treeview(found_frame, columns=columns, show="headings", selectmode="extended")
        self.tree.heading("official_name", text="Official Mod Name")
        self.tree.heading("status", text="Status")
        self.tree.heading("progress", text="Progress")
        self.tree.column("official_name", width=200, stretch=tk.YES)
        self.tree.column("status", width=140, anchor="center")
        self.tree.column("progress", width=100, anchor="center")
        self.tree.tag_configure('found', foreground='green')
        self.tree.tag_configure('fallback', foreground='#E67E22')
        self.tree.tag_configure('dependency', foreground='#3498DB')
        self.tree.tag_configure('downloading', foreground='blue')
        self.tree.pack(fill="both", expand=True)
        
        self.not_found_tree = ttk.Treeview(not_found_frame, columns=(), show="tree")
        self.not_found_tree.pack(fill="both", expand=True)
        
        button_frame = ttk.Frame(main_frame)
        button_frame.pack(fill="x", pady=5)
        self.search_button = ttk.Button(button_frame, text="Search Mods", command=self.start_search_thread)
        self.search_button.pack(side="left")
        self.research_button = ttk.Button(button_frame, text="Re-Search Not Found", command=self.start_research_not_found_thread)
        self.research_button.pack(side="left", padx=5)
        self.download_all_button = ttk.Button(button_frame, text="Download All Available", command=self.start_download_all_thread)
        self.download_all_button.pack(side="right", padx=(0, 5))
        self.download_selected_button = ttk.Button(button_frame, text="Download Selected", command=self.start_download_selected_thread)
        self.download_selected_button.pack(side="right")
        
        self.action_buttons = [self.search_button, self.research_button, self.download_all_button, self.download_selected_button]

        status_bar_frame = ttk.Frame(main_frame, relief="sunken", padding=2)
        status_bar_frame.pack(side="bottom", fill="x")
        self.status_label = ttk.Label(status_bar_frame, text="Ready")
        self.status_label.pack(side="left")

        github_button = ttk.Button(status_bar_frame, text="View on GitHub", command=self.open_github)
        github_button.pack(side="right", padx=(2, 2))
        paypal_button = ttk.Button(status_bar_frame, text="Donate", command=self.open_paypal)
        paypal_button.pack(side="right", padx=(0, 2))

    def toggle_theme(self):
        if self.dark_mode_var.get():
            sv_ttk.set_theme("dark")
        else:
            sv_ttk.set_theme("light")
        self.update_widget_colors()

    def update_widget_colors(self):
        style = ttk.Style()
        bg_color = style.lookup('TFrame', 'background')
        fg_color = style.lookup('TLabel', 'foreground')
        self.text_input.config(background=bg_color, foreground=fg_color)
    
    def open_github(self):
        webbrowser.open_new_tab(GITHUB_URL)

    def open_paypal(self):
        webbrowser.open_new_tab(PAYPAL_URL)
    
    def show_resolving_popup(self):
        self.resolving_popup = tk.Toplevel(self.root)
        self.resolving_popup.title("Working...")
        self.resolving_popup.transient(self.root)
        self.resolving_popup.grab_set()
        self.resolving_popup.resizable(False, False)
        ttk.Label(self.resolving_popup, text="Resolving dependencies, please wait...", padding=20).pack()
        progress = ttk.Progressbar(self.resolving_popup, mode="indeterminate", length=300)
        progress.pack(padx=20, pady=(0, 20))
        progress.start(10)
        self.root.update_idletasks()
        x = self.root.winfo_x() + (self.root.winfo_width() // 2) - (self.resolving_popup.winfo_width() // 2)
        y = self.root.winfo_y() + (self.root.winfo_height() // 2) - (self.resolving_popup.winfo_height() // 2)
        self.resolving_popup.geometry(f"+{x}+{y}")

    def hide_resolving_popup(self):
        if self.resolving_popup:
            self.resolving_popup.destroy()
            self.resolving_popup = None

    def toggle_buttons(self, enabled):
        state = "normal" if enabled else "disabled"
        for btn in self.action_buttons:
            btn.config(state=state)

    def update_status_bar(self, text):
        self.root.after(0, lambda: self.status_label.config(text=text))

    def browse_dir(self):
        directory = filedialog.askdirectory(title="Select Download Directory")
        if directory: self.dir_var.set(directory)

    def import_mods_from_folder(self):
        folder_path = filedialog.askdirectory(title="Select Folder to Import Mods From")
        if not folder_path: return
        mod_names = set()
        try:
            for filename in os.listdir(folder_path):
                if os.path.isfile(os.path.join(folder_path, filename)) and filename.lower().endswith(".jar"):
                    clean_name = Path(filename).stem
                    clean_name = re.sub(r'[-_](v)?(\d+\.)?(\d+\.)?(\*|\d+).*$', '', clean_name, count=1)
                    loader_suffixes = ['-forge', '_forge', '-fabric', '_fabric', '-neoforge', '_neoforge', '-quilt', '_quilt']
                    for suffix in loader_suffixes:
                        if clean_name.lower().endswith(suffix):
                            clean_name = clean_name[:-len(suffix)]
                            break
                    clean_name = re.sub(r'[-_]', ' ', clean_name).strip()
                    if clean_name: mod_names.add(clean_name)
        except Exception as e:
            messagebox.showerror("Error Reading Folder", f"Could not read files.\n\nError: {e}")
            return
        if mod_names:
            self.text_input.delete("1.0", tk.END)
            self.text_input.insert("1.0", "\n".join(sorted(list(mod_names))))
            messagebox.showinfo("Import Complete", f"Imported {len(mod_names)} names.")
        else:
            messagebox.showwarning("Import Failed", "No .jar files found.")

    def start_search_thread(self):
        mod_names = [line.strip() for line in self.text_input.get("1.0", "end").splitlines() if line.strip()]
        if not mod_names:
            messagebox.showwarning("Input Missing", "Please enter or import mod names.")
            return
        self.tree.delete(*self.tree.get_children())
        self.not_found_tree.delete(*self.not_found_tree.get_children())
        self.results.clear()
        self._start_search_for(mod_names)

    def start_research_not_found_thread(self):
        not_found_items = self.not_found_tree.get_children()
        if not not_found_items:
            messagebox.showinfo("Information", "The 'Not Found' list is empty.")
            return
        mod_names_to_research = [self.not_found_tree.item(item_id, 'text') for item_id in not_found_items]
        for item_id in not_found_items:
            self.not_found_tree.delete(item_id)
        self._start_search_for(mod_names_to_research)

    def _start_search_for(self, mod_names):
        self.total_searched = len(mod_names)
        thread = threading.Thread(target=self.search_worker_manager, args=(mod_names,))
        thread.daemon = True
        thread.start()

    def search_worker_manager(self, mod_names):
        threads = []
        loader, version = self.loader_var.get(), self.version_entry.get()
        for name in mod_names:
            t = threading.Thread(target=self.find_one_mod_worker, args=(name, loader, version))
            t.daemon = True
            t.start()
            threads.append(t)
        for t in threads:
            t.join()
        found_count = len(self.tree.get_children()) - len([i for i in self.tree.get_children() if self.tree.item(i)['values'][0].startswith('Dependency')])
        self.update_status_bar(f"Search complete. Found {found_count} of {self.total_searched} mods.")

    def find_one_mod_worker(self, name, loader, version):
        self.update_status_bar(f"Searching for '{name}' (API)...")
        result = self._fetch_mod_data_from_search(name, loader, version)
        if result:
            self.results[name] = result
            self.root.after(0, self.add_to_found_list, name, result, "Available (API)", ('found',))
            return
        cleaned_name = re.sub(r'\d+$', '', name)
        if cleaned_name != name:
            self.update_status_bar(f"Searching for '{cleaned_name}' (API)...")
            result = self._fetch_mod_data_from_search(cleaned_name, loader, version)
            if result:
                self.results[name] = result
                self.root.after(0, self.add_to_found_list, name, result, "Available (API)", ('found',))
                return
        self.update_status_bar(f"Searching for '{name}' (Fallback)...")
        result = self._search_modrinth_fallback(name, loader, version)
        if result:
            self.results[name] = result
            self.root.after(0, self.add_to_found_list, name, result, "Available (Fallback)", ('fallback',))
        else:
            self.root.after(0, self.add_to_not_found_list, name)

    def add_to_found_list(self, original_name, result, status_prefix, tags):
        version_type = result.get('version_type', 'release')
        final_status = f"{status_prefix} ({version_type})"
        self.tree.insert("", "end", iid=original_name, values=(result['name'], final_status, ""), tags=tags)

    def add_to_not_found_list(self, mod_name):
        self.not_found_tree.insert("", "end", text=mod_name)

    def _generate_potential_slugs(self, mod_name):
        name_no_version = re.sub(r'\d+$', '', mod_name)
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1-\2', name_no_version)
        base_slug = re.sub('([a-z0-9])([A-Z])', r'\1-\2', s1).lower().replace(" ", "-")
        return [base_slug, base_slug + "-fabric", base_slug + "-forge"]

    def _fetch_mod_data_from_search(self, query, loader, game_version):
        try:
            self.rate_limiter.wait()
            params = {"query": query, "limit": 5, "facets": f'[["project_type:mod"]]'}
            res = requests.get(f"{MODRINTH_API_BASE}/search", params=params, timeout=10)
            res.raise_for_status()
            hits = res.json().get("hits", [])
            for hit in hits:
                mod_info = self.get_mod_info(hit["project_id"], loader, game_version)
                if mod_info:
                    return mod_info
            return None
        except requests.RequestException:
            return None

    def _search_modrinth_fallback(self, mod_name, loader, game_version):
        for slug in self._generate_potential_slugs(mod_name):
            mod_info = self._fetch_mod_data_from_slug(slug, loader, game_version)
            if mod_info: return mod_info
        return None

    def _fetch_mod_data_from_slug(self, slug, loader, game_version):
        try:
            self.rate_limiter.wait()
            proj_url = f"{MODRINTH_API_BASE}/project/{slug}"
            proj_res = requests.get(proj_url, timeout=10)
            if proj_res.status_code != 200: return None
            return self.get_mod_info(proj_res.json()["id"], loader, game_version)
        except requests.RequestException:
            return None
    
    def get_mod_info(self, project_id, loader, game_version):
        try:
            self.rate_limiter.wait()
            proj_url = f"{MODRINTH_API_BASE}/project/{project_id}"
            proj_resp = requests.get(proj_url, timeout=10)
            proj_resp.raise_for_status()
            proj_data = proj_resp.json()
            ver_url = f"{MODRINTH_API_BASE}/project/{project_id}/version"
            for version_type in ["release", "beta", "alpha"]:
                self.rate_limiter.wait()
                version_params = {"loaders": f'["{loader}"]', "game_versions": f'["{game_version}"]'}
                ver_resp = requests.get(ver_url, params=version_params, timeout=10)
                ver_resp.raise_for_status()
                versions = ver_resp.json()
                for v in versions:
                    if v['version_type'] == version_type:
                        primary_file = next((f for f in v['files'] if f['primary']), v['files'][0])
                        return {"name": proj_data["title"], "project_id": project_id, "dependencies": v.get("dependencies", []), "download_url": primary_file["url"], "filename": primary_file["filename"], "version_type": version_type}
            return None
        except requests.RequestException:
            return None

    def start_download_selected_thread(self):
        selected_items = self.tree.selection()
        if not selected_items: messagebox.showwarning("No Selection", "Please select mods to download.")
        else: self._initiate_downloads(selected_items)

    def start_download_all_thread(self):
        all_item_ids = self.tree.get_children()
        if not all_item_ids: messagebox.showwarning("Nothing to Download", "No mods are available.")
        else: self._initiate_downloads(all_item_ids)

    def _initiate_downloads(self, item_ids):
        dest_folder = Path(self.dir_var.get())
        dest_folder.mkdir(parents=True, exist_ok=True)
        self.downloading_set.clear()
        self.toggle_buttons(False)
        self.show_resolving_popup()
        self.update_status_bar("Resolving dependencies...")
        mods_to_download = [self.results[item_id] for item_id in item_ids if item_id in self.results]
        thread = threading.Thread(target=self.resolve_and_download_all, args=(mods_to_download, dest_folder))
        thread.daemon = True
        thread.start()

    def resolve_and_download_all(self, initial_mods, dest_folder):
        download_queue = []
        seen_projects = set()
        def resolve(mod_info):
            if mod_info['project_id'] in seen_projects: return
            seen_projects.add(mod_info['project_id'])
            for dep in mod_info.get('dependencies', []):
                if dep['dependency_type'] == 'required':
                    self.root.after(0, lambda name=mod_info['name']: self.update_status_bar(f"Resolving dependency for {name}..."))
                    dep_info = self.get_mod_info(dep['project_id'], self.loader_var.get(), self.version_entry.get())
                    if dep_info: resolve(dep_info)
            download_queue.append(mod_info)
        for mod in initial_mods:
            resolve(mod)
        self.root.after(0, self.hide_resolving_popup)
        if not download_queue:
            self.root.after(0, lambda: messagebox.showinfo("Complete", "No new mods or dependencies to download."))
            self.root.after(0, self.toggle_buttons, True)
            self.update_status_bar("Ready")
            return
        total_downloads = len(download_queue)
        download_threads = []
        for i, mod_to_download in enumerate(download_queue):
            if mod_to_download['project_id'] not in self.downloading_set:
                self.downloading_set.add(mod_to_download['project_id'])
                self.update_status_bar(f"Starting download {i+1}/{total_downloads}: {mod_to_download['name']}")
                dl_thread = threading.Thread(target=self.download_mod, args=(mod_to_download, dest_folder))
                dl_thread.daemon = True
                download_threads.append(dl_thread)
        for thread in download_threads:
            thread.start()
            thread.join()
        self.root.after(0, self.toggle_buttons, True)
        self.update_status_bar("All downloads completed.")

    def download_mod(self, mod_data, dest_folder):
        iid = next((k for k, v in self.results.items() if v and v['project_id'] == mod_data['project_id']), mod_data['name'])
        if not self.tree.exists(iid):
            status = f"Dependency ({mod_data.get('version_type', 'release')})"
            self.root.after(0, lambda: self.tree.insert("", "end", iid=iid, values=(mod_data['name'], status, ""), tags=('dependency',)))
        self.root.after(0, self.update_download_progress, iid, 0)
        url, filename = mod_data['download_url'], mod_data['filename']
        filepath = dest_folder / filename
        try:
            with requests.get(url, stream=True, timeout=30) as r:
                r.raise_for_status()
                total_size = int(r.headers.get('content-length', 0))
                downloaded_size = 0
                last_update_time = time.time()
                with open(filepath, 'wb') as f:
                    for chunk in r.iter_content(chunk_size=8192):
                        f.write(chunk)
                        downloaded_size += len(chunk)
                        current_time = time.time()
                        if total_size > 0 and (current_time - last_update_time) > 0.1:
                            progress = int((downloaded_size / total_size) * 100)
                            self.root.after(0, self.update_download_progress, iid, progress)
                            last_update_time = current_time
            self.root.after(0, self.update_download_progress, iid, 100)
        except Exception as e:
            error_message = str(e)
            if isinstance(e, requests.exceptions.RequestException):
                error_message = "Network Error"
            self.root.after(0, self.update_download_error, iid, error_message)
            if filepath.exists(): 
                try:
                    os.remove(filepath)
                except OSError:
                    pass

    def update_download_progress(self, iid, progress):
        if self.tree.exists(iid):
            current_values = list(self.tree.item(iid, 'values'))
            current_values[2] = f"{progress}%"
            tags = list(self.tree.item(iid, 'tags'))
            if 'downloading' not in tags:
                tags.append('downloading')
            self.tree.item(iid, values=tuple(current_values), tags=tuple(tags))

    def update_download_error(self, iid, error_message):
        if self.tree.exists(iid):
            current_values = list(self.tree.item(iid, 'values'))
            current_values[1] = "Error"
            current_values[2] = error_message[:20]
            self.tree.item(iid, values=tuple(current_values))


if __name__ == "__main__":
    root = tk.Tk()
    app = CraftPackerApp(root)
    root.mainloop()

