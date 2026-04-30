import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Nemo', '3.0')
gi.require_version('Poppler', '0.18')
from gi.repository import Gtk, Nemo, GObject, GLib, GdkPixbuf, Poppler
import os, subprocess, tempfile

# ── Logging ────────────────────────────────────────────────────────────────────
_log_dir  = os.path.expanduser("~/.local/state/nemo-preview")
_log_path = os.path.join(_log_dir, "debug.log")
os.makedirs(_log_dir, exist_ok=True)

def log(msg):
    with open(_log_path, "a") as f:
        f.write(msg + "\n")

# ── Shared state across both extension classes ─────────────────────────────────
_previews = {}   # window → PreviewWidget
_states   = {}   # window → PanelState

# ── Icon config ────────────────────────────────────────────────────────────────
ICON_OPEN  = "go-previous-symbolic"
ICON_CLOSE = "go-next-symbolic"

MENU_LABEL   = "Preview Panel"
ACCEL_KEY    = "p"
ACCEL_MOD    = "<Alt>"
ACCEL_STRING = f"{ACCEL_MOD}{ACCEL_KEY}"

IMAGE_EXTS  = {'.png','.jpg','.jpeg','.gif','.bmp','.tiff','.tif','.webp','.svg'}
PDF_EXTS    = {'.pdf'}
OFFICE_EXTS = {'.odt','.ods','.odp','.doc','.docx','.xls','.xlsx','.ppt','.pptx'}


# ══════════════════════════════════════════════════════════════════════════════
# Renderers
# ══════════════════════════════════════════════════════════════════════════════

class ImageRenderer(Gtk.ScrolledWindow):
    def __init__(self):
        super().__init__()
        self.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        self._image = Gtk.Image()
        self.add(self._image)

    def load(self, path, width):
        try:
            pb = GdkPixbuf.Pixbuf.new_from_file_at_scale(path, width, -1, True)
            self._image.set_from_pixbuf(pb)
            log(f"[ImageRenderer] loaded {path}")
        except Exception as e:
            log(f"[ImageRenderer] ERROR: {e}")


class PdfRenderer(Gtk.ScrolledWindow):
    def __init__(self):
        super().__init__()
        self.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        self._box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
        self.add(self._box)

    def load(self, path, width):
        for child in self._box.get_children():
            self._box.remove(child)
            child.destroy()

        try:
            import cairo
            import numpy as np

            doc = Poppler.Document.new_from_file(f"file://{path}")
            n   = doc.get_n_pages()
            log(f"[PdfRenderer] {n} pages in {path}")

            for i in range(min(n, 20)):
                page   = doc.get_page(i)
                pw, ph = page.get_size()
                scale  = width / pw
                w, h   = int(pw * scale), int(ph * scale)

                surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, w, h)
                ctx = cairo.Context(surface)
                ctx.scale(scale, scale)
                ctx.set_source_rgb(1, 1, 1)
                ctx.paint()
                page.render(ctx)

                surface.flush()  # sicherstellen dass Cairo fertig geschrieben hat
                arr = np.frombuffer(surface.get_data(), dtype=np.uint8).reshape((h, w, 4))
                out = arr[:, :, [2, 1, 0]].tobytes()  # BGRA → RGB

                pb = GdkPixbuf.Pixbuf.new_from_bytes(
                    GLib.Bytes.new(out),
                    GdkPixbuf.Colorspace.RGB, False, 8, w, h, w * 3)
                img = Gtk.Image.new_from_pixbuf(pb)
                self._box.pack_start(img, False, False, 2)

            self._box.show_all()

        except Exception as e:
            log(f"[PdfRenderer] ERROR: {e}")

class OfficeRenderer(Gtk.ScrolledWindow):
    def __init__(self):
        super().__init__()
        self.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        self._image = Gtk.Image()
        self._label = Gtk.Label(label="Converting...")
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        box.pack_start(self._label, True, True, 0)
        box.pack_start(self._image, True, True, 0)
        self.add(box)
        self._tmpdir = tempfile.mkdtemp(prefix="nemo-preview-")

    def load(self, path, width):
        self._label.set_text("Converting with LibreOffice...")
        self._image.clear()
        GLib.idle_add(self._convert, path, width)

    def _convert(self, path, width):
        try:
            result = subprocess.run([
                "libreoffice", "--headless", "--convert-to", "png",
                "--outdir", self._tmpdir, path
            ], capture_output=True, timeout=30)
            base = os.path.splitext(os.path.basename(path))[0] + ".png"
            out  = os.path.join(self._tmpdir, base)
            if os.path.exists(out):
                pb = GdkPixbuf.Pixbuf.new_from_file_at_scale(out, width, -1, True)
                self._image.set_from_pixbuf(pb)
                self._label.hide()
                log(f"[OfficeRenderer] rendered {out}")
            else:
                self._label.set_text("Conversion failed")
        except Exception as e:
            self._label.set_text(f"Error: {e}")
            log(f"[OfficeRenderer] ERROR: {e}")
        return False

def is_binary(path, sample=8192):
    try:
        with open(path, 'rb') as f:
            return b'\x00' in f.read(sample)
    except Exception:
        return True


class TextRenderer(Gtk.ScrolledWindow):
    def __init__(self):
        super().__init__()
        self.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        gi.require_version('GtkSource', '4')
        from gi.repository import GtkSource
        self._lm  = GtkSource.LanguageManager.get_default()
        self._buf = GtkSource.Buffer()
        self._buf.set_highlight_syntax(True)
        self._view = GtkSource.View.new_with_buffer(self._buf)
        self._view.set_editable(False)
        self._view.set_monospace(True)
        self._view.set_show_line_numbers(True)
        self._view.set_wrap_mode(Gtk.WrapMode.NONE)
        self.add(self._view)

    def load(self, path):
        try:
            lang = self._lm.guess_language(path, None)
            self._buf.set_language(lang)
            with open(path, 'r', errors='replace') as f:
                self._buf.set_text(f.read(1024 * 512))  # max 512KB
            log(f"[TextRenderer] loaded {path} lang={lang.get_name() if lang else 'none'}")
        except Exception as e:
            log(f"[TextRenderer] ERROR: {e}")


class PreviewWidget(Gtk.Box):
    def __init__(self):
        super().__init__(orientation=Gtk.Orientation.VERTICAL)
        self._stack   = Gtk.Stack()
        self._img_r   = ImageRenderer()
        self._pdf_r   = PdfRenderer()
        self._off_r   = OfficeRenderer()
        self._txt_r   = TextRenderer()
        self._no_prev = Gtk.Label(label="No preview available")
        self._stack.add_named(self._img_r,   "image")
        self._stack.add_named(self._pdf_r,   "pdf")
        self._stack.add_named(self._off_r,   "office")
        self._stack.add_named(self._txt_r,   "text")
        self._stack.add_named(self._no_prev, "none")
        self.pack_start(self._stack, True, True, 0)
        self.show_all()

    def load(self, path):
        if not path or not os.path.isfile(path):
            self._stack.set_visible_child_name("none")
            return
        ext   = os.path.splitext(path)[1].lower()
        width = max(self.get_allocated_width() - 16, 200)
        log(f"[PreviewWidget] loading {path} ext={ext} width={width}")
        if ext in IMAGE_EXTS:
            self._img_r.load(path, width)
            self._stack.set_visible_child_name("image")
        elif ext in PDF_EXTS:
            self._pdf_r.load(path, width)
            self._stack.set_visible_child_name("pdf")
        elif ext in OFFICE_EXTS:
            self._off_r.load(path, width)
            self._stack.set_visible_child_name("office")
        else:
            if not is_binary(path):
                self._txt_r.load(path)
                self._stack.set_visible_child_name("text")
            else:
                self._stack.set_visible_child_name("none")


# ══════════════════════════════════════════════════════════════════════════════
# PanelState
# ══════════════════════════════════════════════════════════════════════════════

class PanelState:
    def __init__(self):
        self.open         = False
        self.disabled     = False
        self.open_before_disable = False  # Zustand vor Tab-Mode
        self.button       = None
        self.menuitem     = None
        self.on_toggle    = None
        self.last_path    = None
        self.loaded_path  = None

    def toggle(self, *_):
        if self.disabled:
            return
        self.open = not self.open
        log(f"[PanelState] toggle → open={self.open}")
        self._sync()
        if self.on_toggle:
            self.on_toggle(self.open)

    def set_disabled(self, disabled):
        if disabled and not self.disabled:
            # Tab-Mode wird aktiv: Zustand merken, Panel schließen
            self.open_before_disable = self.open
            self.disabled = True
            if self.open:
                self.open = False
                self._sync()
                if self.on_toggle:
                    self.on_toggle(False)
            else:
                self._sync()
        elif not disabled and self.disabled:
            # Tab-Mode endet: Zustand wiederherstellen
            self.disabled = False
            self._sync()
            # Panel nur wiederherstellen wenn es vorher offen war
            if self.open_before_disable:
                self.open = True
                self._sync()
                if self.on_toggle:
                    self.on_toggle(True)

    def _sync(self):
        icon = ICON_OPEN if self.open else ICON_CLOSE
        if self.button:
            self.button.set_image(
                Gtk.Image.new_from_icon_name(icon, Gtk.IconSize.SMALL_TOOLBAR))
            self.button.set_sensitive(not self.disabled)
        if self.menuitem:
            self.menuitem.handler_block_by_func(self.toggle)
            self.menuitem.set_active(self.open)
            self.menuitem.set_sensitive(not self.disabled)
            self.menuitem.handler_unblock_by_func(self.toggle)


# ══════════════════════════════════════════════════════════════════════════════
# Extension class 1: UI + Panel
# ══════════════════════════════════════════════════════════════════════════════

class PreviewPanelLocation(GObject.GObject, Nemo.LocationWidgetProvider):

    def __init__(self):
        self._states = {}

    def get_widget(self, uri, window):
        if window not in self._states:
            log("[PreviewPanel] setting up window")
            state = PanelState()
            self._states[window] = state
            self._inject_toolbar_button(window, state)
            self._inject_menu_item(window, state)
            self._register_shortcut(window, state)
            self._watch_tabs(window, state)
            GLib.idle_add(self._inject_preview_panel, window, state)
        return None

    def _inject_toolbar_button(self, window, state):
        try:
            grid      = window.get_children()[0]
            box_outer = [c for c in grid.get_children() if isinstance(c, Gtk.Box)][0]
            toolbar_w = [c for c in box_outer.get_children() if type(c).__name__ == 'NemoToolbar'][0]
            gtktbar   = toolbar_w.get_children()[0]
            toolitem  = gtktbar.get_children()[2]
            btn_box   = toolitem.get_children()[0]
            btn = Gtk.Button()
            btn.set_relief(Gtk.ReliefStyle.NONE)
            btn.set_image(Gtk.Image.new_from_icon_name(ICON_CLOSE, Gtk.IconSize.SMALL_TOOLBAR))
            btn.set_tooltip_text(f"Toggle Preview Panel ({ACCEL_MOD}{ACCEL_KEY.upper()})")
            btn.connect("clicked", state.toggle)
            btn.show_all()
            btn_box.pack_end(btn, False, False, 2)
            state.button = btn
            log("[PreviewPanel] toolbar button injected")
        except Exception as e:
            log(f"[PreviewPanel] ERROR injecting toolbar button: {e}")

    def _inject_menu_item(self, window, state):
        try:
            menubar = self._find_menubar(window)
            if not menubar:
                return
            view_menu = None
            for item in menubar.get_children():
                if item.get_name() == "View":
                    view_menu = item.get_submenu()
                    break
            if not view_menu:
                return
            sep = Gtk.SeparatorMenuItem()
            sep.show()
            view_menu.append(sep)
            mi = Gtk.CheckMenuItem(label=MENU_LABEL)
            mi.show()
            mi.connect("toggled", state.toggle)
            view_menu.append(mi)
            state.menuitem = mi
            log("[PreviewPanel] menu item injected")
        except Exception as e:
            log(f"[PreviewPanel] ERROR injecting menu item: {e}")

    def _find_menubar(self, window):
        for c in window.get_children()[0].get_children():
            if type(c).__name__ == 'MenuBar':
                return c
        return None

    def _register_shortcut(self, window, state):
        try:
            accel_group = Gtk.AccelGroup()
            key, mod = Gtk.accelerator_parse(ACCEL_STRING)
            accel_group.connect(key, mod, Gtk.AccelFlags.VISIBLE, lambda *_: state.toggle())
            window.add_accel_group(accel_group)
            log(f"[PreviewPanel] shortcut {ACCEL_STRING} registered")
        except Exception as e:
            log(f"[PreviewPanel] ERROR registering shortcut: {e}")

    def _watch_tabs(self, window, state):
        notebook = self._find_notebook(window)
        if not notebook:
            return
        notebook.connect("page-added",   self._on_tabs_changed, state)
        notebook.connect("page-removed", self._on_tabs_changed, state)

    def _on_tabs_changed(self, notebook, _page, _num, state):
        state.set_disabled(notebook.get_n_pages() > 1)

    def _find_notebook(self, window):
        def walk(w):
            if type(w).__name__ == 'NemoNotebook':
                return w
            if hasattr(w, 'get_children'):
                for c in w.get_children():
                    r = walk(c)
                    if r:
                        return r
            return None
        return walk(window)

    def _inject_preview_panel(self, window, state):
        log("[PreviewPanel] _inject_preview_panel called")
        try:
            inner_paned = self._find_inner_paned(window)
            if not inner_paned:
                log("[PreviewPanel] ERROR: inner paned not found")
                return False
            nemo_pane = inner_paned.get_child1()
            if not nemo_pane:
                log("[PreviewPanel] ERROR: child1 is None")
                return False
            inner_paned.remove(nemo_pane)
            split   = Gtk.Paned(orientation=Gtk.Orientation.HORIZONTAL)
            preview = PreviewWidget()
            split.pack1(nemo_pane, resize=True,  shrink=False)
            split.pack2(preview,   resize=False, shrink=False)
            split.show_all()
            preview.hide()
            inner_paned.pack1(split, resize=True, shrink=False)
            split.connect("notify::position", self._on_resize)
            _previews[window] = preview
            _states[window]   = state
            log(f"[PreviewPanel] registered preview for window, total={len(_previews)}")

            def on_toggle(open):
                log(f"[PreviewPanel] on_toggle open={open} last={state.last_path} loaded={state.loaded_path}")
                if open:
                    preview.show_all()
                    if not state.loaded_path:
                        w = split.get_allocated_width()
                        if w > 1:
                            split.set_position(int(w * 0.5))
                    if state.last_path and state.last_path != state.loaded_path:
                        log(f"[PreviewPanel] loading on open: {state.last_path}")
                        preview.load(state.last_path)
                        state.loaded_path = state.last_path
                else:
                    preview.hide()
                log(f"[PreviewPanel] panel {'opened' if open else 'closed'}")
            state.on_toggle = on_toggle
            log("[PreviewPanel] injection complete")
        except Exception as e:
            log(f"[PreviewPanel] ERROR: {e}")
        return False

    def _on_resize(self, paned, _param):
        log(f"[PreviewPanel] panel width: {paned.get_allocation().width - paned.get_position()}px")

    def _find_inner_paned(self, window):
        def walk(w, depth=0):
            if isinstance(w, Gtk.Paned) and depth > 0:
                for c in [w.get_child1(), w.get_child2()]:
                    if c and type(c).__name__ == 'NemoWindowPane':
                        return w
            if hasattr(w, 'get_children'):
                for c in w.get_children():
                    r = walk(c, depth + 1)
                    if r:
                        return r
            return None
        return walk(window)


# ══════════════════════════════════════════════════════════════════════════════
# Extension class 2: Selection via NemoSelectionProvider
# ══════════════════════════════════════════════════════════════════════════════

class PreviewPanelSelection(GObject.GObject, Nemo.SelectionProvider):

    def selection_changed(self, window, files):
        if not files or window not in _previews:
            return
        state = _states.get(window)
        if not state:
            return
        path = files[0].get_location().get_path()
        if path:
            state.last_path = path
        if not state.open:
            return
        if path:
            log(f"[PreviewSelection] loading: {path}")
            _previews[window].load(path)
            state.loaded_path = path
