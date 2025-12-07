#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Official repo: https://github.com/meowrch/nemo-extensions
# WARNING: этот файл сгенерирован автоматически build.py
# Не редактируйте его вручную — вносите изменения в src/*.py и пересобирайте.

# ===== BEGIN src/header.py =====

from pathlib import Path

import gi

gi.require_version("Nemo", "3.0")
gi.require_version("Gtk", "3.0")
gi.require_version("Gdk", "3.0")

DB_DIR = Path.home() / ".local" / "share" / "nemo-tags"
DB_PATH = DB_DIR / "tags.json"
ICONS_DIR = DB_DIR / "icons"
VIEWS_DIR = DB_DIR / "views"
EMBLEMS_DIR = DB_DIR / "emblems"

# ===== END src/header.py =====

# ===== BEGIN src/icons.py =====

import itertools
import threading
from pathlib import Path
from typing import List, Callable, Optional

import shutil
import subprocess

from gi.repository import GdkPixbuf, Gtk, GLib



class TagIconGenerator:
    """Generate colored circle icons for tags"""
    
    # Кеш для отслеживания уже созданных эмблем
    _created_emblems = set()
    _lock = threading.Lock()

    # Флаг, что обновление кеша уже запланировано
    _cache_update_scheduled = False

    @staticmethod
    def hex_to_rgb(hex_color: str) -> tuple:
        """Convert #RRGGBB to (r, g, b) float tuple"""
        hex_color = hex_color.lstrip("#")
        return tuple(int(hex_color[i : i + 2], 16) / 255.0 for i in (0, 2, 4))

    @staticmethod
    def create_tag_icon(color: str, size: int = 16) -> str:
        """Return path to SVG emblem for a single-color tag.

        Для UI (панель тегов, контекстное меню) просто используем SVG-эмблему
        с одним кружком и даём GTK самому масштабировать её до нужного размера.
        """
        emblem_name = TagIconGenerator.create_emblem_icon([color])
        emblem_path = EMBLEMS_DIR / f"{emblem_name}.svg"
        return str(emblem_path)

    @staticmethod
    def pregenerate_all_combinations(tags: List[dict], callback: Optional[Callable] = None):
        """Предварительно генерируем все возможные комбинации эмблем тегов.
        
        Генерация теперь идёт в главном GTK-потоке маленькими порциями через GLib.idle_add,
        чтобы не использовать GTK из фонового потока и не создавать гонки при записи файлов.
        
        Args:
            tags: Список тегов
            callback: Функция, которая будет вызвана после завершения генерации (в главном потоке GTK)
        """
        if not tags:
            if callback:
                GLib.idle_add(callback)
            return

        colors = [tag["color"] for tag in tags if "color" in tag]
        if not colors:
            if callback:
                GLib.idle_add(callback)
            return

        # Строим список всех комбинаций заранее
        combos: List[List[str]] = []
        for r in range(1, min(4, len(colors) + 1)):
            for combo in itertools.combinations(colors, r):
                combos.append(list(combo))

        if not combos:
            if callback:
                GLib.idle_add(callback)
            return

        print(f"[TagIconGenerator] Pregenerating emblems for {len(colors)} tags ({len(combos)} combos)...")

        iterator = iter(combos)

        def process_batch(it):
            # Обрабатываем по несколько комбинаций за один проход, чтобы не блокировать UI
            try:
                for _ in range(5):  # до 5 эмблем за один idle
                    colors_list = next(it)
                    TagIconGenerator.create_emblem_icon(colors_list, skip_cache_update=True)
                return True  # продолжить вызывать idle
            except StopIteration:
                print("[TagIconGenerator] Pregeneration complete!")

                def on_done():
                    # Планируем единичное обновление кеша после завершения генерации
                    TagIconGenerator.schedule_icon_cache_update()
                    if callback:
                        callback()
                    return False

                GLib.idle_add(on_done)
                return False  # остановить idle

        GLib.idle_add(process_batch, iterator)

    @staticmethod
    def create_emblem_icon(colors: List[str], skip_cache_update: bool = False) -> str:
        """Create elegant stacked emblem icon SVG for file/folder badges
        
        Args:
            colors: Список цветов для эмблемы
            skip_cache_update: Если True, не обновляет кеш GTK (используется при массовой генерации)
        """
        EMBLEMS_DIR.mkdir(parents=True, exist_ok=True)

        colors_to_use = colors[-3:] if len(colors) > 3 else colors

        color_signature = "-".join([c.replace("#", "") for c in colors_to_use])
        emblem_name = f"nemo-tag-emblem-{color_signature}"
        
        # Проверяем кеш - если эмблема уже создана, не создаём повторно
        with TagIconGenerator._lock:
            if emblem_name in TagIconGenerator._created_emblems:
                return emblem_name
        
        emblem_path = EMBLEMS_DIR / f"{emblem_name}.svg"

        size = 16
        radius = 3
        num_circles = len(colors_to_use)

        svg_circles = []

        if num_circles == 1:
            svg_circles.append(
                f'<circle cx="{size / 2}" cy="{size / 2}" r="{radius}" fill="{colors_to_use[0]}"/>'
            )

        elif num_circles == 2:
            offset = 2.5
            positions = [(size / 2 - offset, size / 2), (size / 2 + offset, size / 2)]

            for i, color in enumerate(colors_to_use):
                svg_circles.append(
                    f'<circle cx="{positions[i][0]}" cy="{positions[i][1]}" r="{radius}" fill="{color}"/>'
                )

        else:
            offset_x = 2.2
            offset_y = 1.8
            positions = [
                (size / 2 - offset_x, size / 2 + offset_y),
                (size / 2 + offset_x, size / 2 + offset_y),
                (size / 2, size / 2 - offset_y),
            ]

            for i, color in enumerate(colors_to_use):
                svg_circles.append(
                    f'<circle cx="{positions[i][0]}" cy="{positions[i][1]}" r="{radius}" fill="{color}"/>'
                )

        with open(emblem_path, "w", encoding="utf-8") as f:
            f.write(f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" viewBox="0 0 {size} {size}">
{"".join(svg_circles)}
</svg>''')

        # Регистрируем эмблему как builtin-иконку в GTK
        try:
            pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_scale(
                str(emblem_path), 16, 16, True
            )
            Gtk.IconTheme.add_builtin_icon(emblem_name, 16, pixbuf)
            
            print(
                f"[TagIconGenerator] Registered emblem: {emblem_name} (colors: {len(colors_to_use)})"
            )
        except Exception as e:
            print(f"Error registering builtin emblem {emblem_name}: {e}")

        # Копируем SVG в актуальную тему иконок
        TagIconGenerator._copy_to_theme(emblem_path, emblem_name)
        
        # Планируем обновление кеша иконок (только если не пропускаем)
        if not skip_cache_update:
            TagIconGenerator.schedule_icon_cache_update()
        
        # Добавляем в кеш созданных эмблем
        with TagIconGenerator._lock:
            TagIconGenerator._created_emblems.add(emblem_name)

        return emblem_name
    
    @staticmethod
    def delete_emblems_with_color(color: str):
        """Удаляет все эмблемы, содержащие указанный цвет.
        
        Удаляет как одноцветные эмблемы, так и многоцветные комбинации,
        содержащие этот цвет.
        
        Args:
            color: Цвет в формате #RRGGBB
        """
        color_sig = color.replace("#", "")
        home = Path.home()
        
        # Получаем список всех директорий с эмблемами
        theme_roots = []
        
        try:
            settings = Gtk.Settings.get_default()
            theme_name = (
                settings.props.gtk_icon_theme_name if settings is not None else None
            )
        except Exception:
            theme_name = None

        if theme_name:
            user_theme_root = home / ".local" / "share" / "icons" / theme_name
            for sub in ("16/emblems", "22/emblems", "scalable/emblems"):
                theme_roots.append(user_theme_root / sub)

        fallback_root = home / ".icons" / "hicolor"
        theme_roots.append(fallback_root / "scalable" / "emblems")
        theme_roots.append(EMBLEMS_DIR)
        
        deleted_count = 0
        
        # Удаляем файлы эмблем, содержащие цвет
        for emblems_dir in theme_roots:
            if not emblems_dir.exists():
                continue
                
            try:
                # Ищем все файлы эмблем с этим цветом
                for emblem_file in emblems_dir.glob(f"nemo-tag-emblem-*{color_sig}*.svg"):
                    try:
                        emblem_file.unlink()
                        deleted_count += 1
                        print(f"[TagIconGenerator] Deleted emblem: {emblem_file.name}")
                        
                        # Удаляем из кеша
                        emblem_name = emblem_file.stem
                        with TagIconGenerator._lock:
                            TagIconGenerator._created_emblems.discard(emblem_name)
                            
                    except Exception as e:
                        print(f"Error deleting emblem {emblem_file}: {e}")
                        
            except Exception as e:
                print(f"Error scanning directory {emblems_dir}: {e}")
        
        if deleted_count > 0:
            print(f"[TagIconGenerator] Deleted {deleted_count} emblem(s) with color {color}")
            # Планируем обновление кеша иконок после удаления
            TagIconGenerator.schedule_icon_cache_update()
    
    @staticmethod
    def _copy_to_theme(emblem_path: Path, emblem_name: str):
        """Копирует эмблему в директории тем иконок"""
        theme_roots = set()
        home = Path.home()

        try:
            settings = Gtk.Settings.get_default()
            theme_name = (
                settings.props.gtk_icon_theme_name if settings is not None else None
            )
        except Exception:
            theme_name = None

        if theme_name:
            user_theme_root = home / ".local" / "share" / "icons" / theme_name
            theme_roots.add(user_theme_root)

            for sub in ("16/emblems", "22/emblems", "scalable/emblems"):
                target_dir = user_theme_root / sub
                try:
                    target_dir.mkdir(parents=True, exist_ok=True)
                    shutil.copy(emblem_path, target_dir / f"{emblem_name}.svg")
                except Exception:
                    pass

        fallback_root = home / ".icons" / "hicolor"
        theme_roots.add(fallback_root)
        fallback_emblems = fallback_root / "scalable" / "emblems"
        try:
            fallback_emblems.mkdir(parents=True, exist_ok=True)
            shutil.copy(emblem_path, fallback_emblems / f"{emblem_name}.svg")
        except Exception:
            pass
    
    @staticmethod
    def _update_icon_cache():
        """Обновляет кеш иконок GTK (вызывать только из главного потока).
        
        Обычно не вызывается напрямую, используйте schedule_icon_cache_update,
        чтобы отложить обновление и коалесцировать несколько запросов.
        """
        home = Path.home()
        theme_roots = set()
        
        try:
            settings = Gtk.Settings.get_default()
            theme_name = (
                settings.props.gtk_icon_theme_name if settings is not None else None
            )
        except Exception:
            theme_name = None

        if theme_name:
            user_theme_root = home / ".local" / "share" / "icons" / theme_name
            theme_roots.add(user_theme_root)

        fallback_root = home / ".icons" / "hicolor"
        theme_roots.add(fallback_root)

        for root in theme_roots:
            if root.exists():
                try:
                    subprocess.run(
                        ["gtk-update-icon-cache", "-f", str(root)],
                        capture_output=True,
                        timeout=5
                    )
                except Exception:
                    pass

    @staticmethod
    def schedule_icon_cache_update(delay_ms: int = 1000):
        """Планирует единичное обновление кеша иконок с задержкой.

        - Всегда выполняется в главном GTK-потоке (через GLib.timeout_add).
        - Если обновление уже запланировано, повторные вызовы ничего не делают.
        Это исключает ситуацию, когда gtk-update-icon-cache вызывается раньше
        фактической записи SVG-файлов и уменьшает количество запусков утилиты.
        """
        with TagIconGenerator._lock:
            if TagIconGenerator._cache_update_scheduled:
                return

            TagIconGenerator._cache_update_scheduled = True

        def do_update():
            try:
                TagIconGenerator._update_icon_cache()
                TagIconGenerator.refresh_icon_theme()
            finally:
                with TagIconGenerator._lock:
                    TagIconGenerator._cache_update_scheduled = False

            return False

        # Отложенный запуск, чтобы гарантировать завершение всех записей файлов
        GLib.timeout_add(delay_ms, do_update)

    @staticmethod
    def clear_cache():
        """Очищает кеш созданных эмблем (для тестирования или при изменении цвета)"""
        with TagIconGenerator._lock:
            TagIconGenerator._created_emblems.clear()
    
    @staticmethod
    def refresh_icon_theme():
        """Принудительно обновляет тему иконок GTK"""
        try:
            icon_theme = Gtk.IconTheme.get_default()
            if icon_theme:
                icon_theme.rescan_if_needed()
        except Exception as e:
            print(f"Error refreshing icon theme: {e}")

# ===== END src/icons.py =====

# ===== BEGIN src/database.py =====

import os
import json
import secrets
from pathlib import Path
from typing import Dict
from typing import List
from typing import Optional



class TagDatabase:
    def __init__(self, db_path: Optional[Path] = None):
        if db_path is None:
            db_path = DB_PATH
        
        self.db_path = db_path
        self.tags: List[Dict] = []
        self.index: Dict[str, List[str]] = {}
        self._load()

    def _load(self):
        if not self.db_path.exists():
            self._save()
        
        try:
            with open(self.db_path, "r", encoding="utf-8") as f:
                data = json.load(f)
        except Exception:
            data = {}
        
        self.tags = data.get("tags", [])
        raw_index = data.get("index", {})

        # Разворачиваем алиасы при загрузке
        self.index = {}
        for tag_id, paths in raw_index.items():
            self.index[tag_id] = [self._expand_path(p) for p in paths]

    def _save(self):
        DB_DIR.mkdir(parents=True, exist_ok=True)
        with open(self.db_path, "w", encoding="utf-8") as f:
            json.dump({"tags": self.tags, "index": self.index}, f, indent=2)

    def _generate_id(self) -> str:
        """Генерирует случайный уникальный идентификатор тега"""
        existing_ids = {t["id"] for t in self.tags}

        while True: # Генерируем 8-символьный hex ID
            tag_id = secrets.token_hex(4)
            if tag_id not in existing_ids:
                return tag_id

    def add_tag(self, name: str, color: str) -> str:
        tag_id = self._generate_id()
        self.tags.append({"id": tag_id, "name": name, "color": color})
        self._save()
        return tag_id

    def get_tags(self) -> List[Dict]:
        return list(self.tags)

    def get_tag_by_id(self, tag_id: str) -> Optional[Dict]:
        for tag in self.tags:
            if tag["id"] == tag_id:
                return tag
        
        return None

    def remove_tag(self, tag_id: str) -> bool:
        before = len(self.tags)
        self.tags = [tag for tag in self.tags if tag["id"] != tag_id]
        self.index.pop(tag_id, None)
        self._save()
        return len(self.tags) != before

    def update_tag(self, tag_id: str, name: str, color: str) -> bool:
        """Обновляет имя и/или цвет тега"""
        for tag in self.tags:
            if tag["id"] == tag_id:
                tag["name"] = name
                tag["color"] = color
                self._save()
                return True

        return False

    def reorder_tags(self, tag_ids: List[str]) -> bool:
        """Изменяет порядок тегов согласно переданному списку ID"""
        if len(tag_ids) != len(self.tags):
            return False

        # Создаём словарь для быстрого поиска
        tag_dict = {tag["id"]: tag for tag in self.tags}

        # Проверяем что все ID существуют
        if not all(tag_id in tag_dict for tag_id in tag_ids):
            return False

        # Переупорядочиваем теги
        self.tags = [tag_dict[tag_id] for tag_id in tag_ids]
        self._save()
        return True

    def assign_tag(self, tag_id: str, file_path: str):
        file_path = str(file_path)
        self.index.setdefault(tag_id, [])

        if file_path not in self.index[tag_id]:
            self.index[tag_id].append(file_path)
            self._save()

    def unassign_tag(self, tag_id: str, file_path: str):
        file_path = str(file_path)
        items = self.index.get(tag_id, [])

        if file_path in items:
            items.remove(file_path)
            self._save()

    def files_by_tag(self, tag_id: str) -> List[str]:
        return list(self.index.get(tag_id, []))

    def tags_for_file(self, file_path: str) -> List[Dict]:
        file_path = str(file_path)
        result = []

        for tag in self.tags:
            if file_path in self.index.get(tag["id"], []):
                result.append(tag)

        return result

    def is_tagged(self, tag_id: str, file_path: str) -> bool:
        file_path = str(file_path)
        return file_path in self.index.get(tag_id, [])

    def flush(self):
        self._save()

    def _expand_path(self, path: str) -> str:
        """Преобразует $HOME и ~ в абсолютный путь"""
        return os.path.expanduser(path.replace("$HOME", "~"))

# ===== END src/database.py =====

# ===== BEGIN src/manager.py =====

import os
import shutil
from pathlib import Path
from typing import Dict, List, Optional



class TagManager:
    def __init__(self, db_path: Optional[Path] = None):
        self.db = TagDatabase(db_path)

    def create_tag(self, name: str, color: str) -> str:
        return self.db.add_tag(name, color)

    def delete_tag(self, tag_id: str):
        return self.db.remove_tag(tag_id)

    def update_tag(self, tag_id: str, name: str, color: str) -> bool:
        """Обновляет имя и/или цвет тега"""
        return self.db.update_tag(tag_id, name, color)

    def reorder_tags(self, tag_ids: List[str]) -> bool:
        """Изменяет порядок тегов"""
        return self.db.reorder_tags(tag_ids)

    def assign_tag_to_file(self, tag_id: str, file_path: str):
        self.db.assign_tag(tag_id, file_path)

    def unassign_tag_from_file(self, tag_id: str, file_path: str):
        self.db.unassign_tag(tag_id, file_path)

    def get_tags(self) -> List[Dict]:
        return self.db.get_tags()

    def get_tag_by_id(self, tag_id: str) -> Optional[Dict]:
        return self.db.get_tag_by_id(tag_id)

    def get_files_by_tag(self, tag_id: str) -> List[str]:
        return self.db.files_by_tag(tag_id)

    def get_tags_for_file(self, file_path: str) -> List[Dict]:
        return self.db.tags_for_file(file_path)

    def is_file_tagged(self, tag_id: str, file_path: str) -> bool:
        return self.db.is_tagged(tag_id, file_path)

    def flush_db(self):
        self.db.flush()

    def create_tag_view(self, tag_id: str, tag_name: str) -> Optional[str]:
        files = self.get_files_by_tag(tag_id)
        if not files:
            return None

        VIEWS_DIR.mkdir(parents=True, exist_ok=True)
        view_dir = VIEWS_DIR / f"tag-{tag_id}"

        if view_dir.exists():
            shutil.rmtree(view_dir)

        view_dir.mkdir()

        for file_path in files:
            if not os.path.exists(file_path):
                continue

            filename = os.path.basename(file_path)
            link_path = view_dir / filename

            counter = 1
            while link_path.exists():
                name, ext = os.path.splitext(filename)
                link_path = view_dir / f"{name}_{counter}{ext}"
                counter += 1

            try:
                os.symlink(file_path, link_path)
            except Exception as e:
                print(f"Error creating symlink: {e}")

        return str(view_dir)

# ===== END src/manager.py =====

# ===== BEGIN src/ui.py =====

import os
import subprocess

from gi.repository import GdkPixbuf, Gtk, Gdk, GLib, Nemo



class TagDialog(Gtk.Dialog):
    def __init__(self, parent=None, edit_mode=False, initial_name="", initial_color="#3498db"):
        title = "Edit Tag" if edit_mode else "Create Tag"
        Gtk.Dialog.__init__(
            self,
            title=title,
            transient_for=parent,
            flags=Gtk.DialogFlags.MODAL,
        )
        self.set_default_size(300, 150)

        self.add_button("_Cancel", Gtk.ResponseType.CANCEL)
        self.add_button("_OK", Gtk.ResponseType.OK)

        box = self.get_content_area()
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        vbox.set_border_width(12)
        box.add(vbox)

        name_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        name_label = Gtk.Label(label="Tag name:")
        name_label.set_xalign(0.0)
        self.name_entry = Gtk.Entry()
        self.name_entry.set_text(initial_name)
        name_box.pack_start(name_label, False, False, 0)
        name_box.pack_start(self.name_entry, True, True, 0)
        vbox.pack_start(name_box, False, False, 0)

        color_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        color_label = Gtk.Label(label="Color:")
        color_label.set_xalign(0.0)
        self.color_button = Gtk.ColorButton()
        
        # Установка начального цвета
        rgba = Gdk.RGBA()
        rgba.parse(initial_color)
        self.color_button.set_rgba(rgba)
        
        color_box.pack_start(color_label, False, False, 0)
        color_box.pack_start(self.color_button, False, False, 0)
        vbox.pack_start(color_box, False, False, 0)

        self.show_all()

    def run_dialog(self):
        response = self.run()
        name = self.name_entry.get_text().strip()
        color_hex = None

        if response == Gtk.ResponseType.OK and name:
            rgba = self.color_button.get_rgba()
            r = int(rgba.red * 255)
            g = int(rgba.green * 255)
            b = int(rgba.blue * 255)
            color_hex = "#{:02X}{:02X}{:02X}".format(r, g, b)

        self.destroy()
        if response == Gtk.ResponseType.OK and name and color_hex:
            return name, color_hex

        return None, None


class RenameTagDialog(Gtk.Window):
    """Плавающее окно для переименования тега"""
    def __init__(self, initial_name=""):
        Gtk.Window.__init__(self, title="Rename Tag")
        self.set_default_size(300, 100)
        self.set_resizable(False)
        self.set_type_hint(Gdk.WindowTypeHint.DIALOG)
        self.set_keep_above(True)
        
        # Для тайлинговых WM
        self.set_decorated(True)
        
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        vbox.set_border_width(15)
        
        # Поле ввода
        self.name_entry = Gtk.Entry()
        self.name_entry.set_text(initial_name)
        self.name_entry.set_placeholder_text("Enter tag name")
        self.name_entry.connect("activate", lambda w: self.on_ok())
        vbox.pack_start(self.name_entry, True, True, 0)
        
        # Кнопки
        button_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        button_box.set_halign(Gtk.Align.END)
        
        cancel_btn = Gtk.Button(label="Cancel")
        cancel_btn.connect("clicked", lambda w: self.on_cancel())
        button_box.pack_start(cancel_btn, False, False, 0)
        
        ok_btn = Gtk.Button(label="OK")
        ok_btn.get_style_context().add_class("suggested-action")
        ok_btn.connect("clicked", lambda w: self.on_ok())
        button_box.pack_start(ok_btn, False, False, 0)
        
        vbox.pack_start(button_box, False, False, 0)
        
        self.add(vbox)
        self.result = None
        
    def on_ok(self):
        self.result = self.name_entry.get_text().strip()
        self.destroy()
        Gtk.main_quit()
        
    def on_cancel(self):
        self.result = None
        self.destroy()
        Gtk.main_quit()
    
    def run(self):
        self.show_all()
        self.name_entry.grab_focus()
        Gtk.main()
        return self.result


class ColorPickerDialog(Gtk.ColorChooserDialog):
    """Диалог выбора цвета"""
    def __init__(self, initial_color="#3498db"):
        Gtk.ColorChooserDialog.__init__(self, title="Choose Color")
        
        rgba = Gdk.RGBA()
        rgba.parse(initial_color)
        self.set_rgba(rgba)
        
    def run_dialog(self):
        response = self.run()
        color_hex = None
        
        if response == Gtk.ResponseType.OK:
            rgba = self.get_rgba()
            r = int(rgba.red * 255)
            g = int(rgba.green * 255)
            b = int(rgba.blue * 255)
            color_hex = "#{:02X}{:02X}{:02X}".format(r, g, b)
        
        self.destroy()
        return color_hex


class TagReorderDialog(Gtk.Dialog):
    """Диалог для изменения порядка тегов"""
    def __init__(self, parent, manager: TagManager):
        Gtk.Dialog.__init__(
            self,
            title="Reorder Tags",
            transient_for=parent,
            flags=Gtk.DialogFlags.MODAL,
        )
        self.manager = manager
        self.set_default_size(400, 300)
        
        # Кнопки
        self.add_button("_Cancel", Gtk.ResponseType.CANCEL)
        save_button = self.add_button("_Save", Gtk.ResponseType.OK)
        save_button.get_style_context().add_class("suggested-action")
        
        # Контент
        box = self.get_content_area()
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        vbox.set_border_width(12)
        box.add(vbox)
        
        # Инструкция
        label = Gtk.Label(label="Drag tags to reorder them:")
        label.set_xalign(0.0)
        vbox.pack_start(label, False, False, 0)
        
        # ScrolledWindow для TreeView
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        scrolled.set_vexpand(True)
        
        # TreeView для списка тегов
        self.store = Gtk.ListStore(str, str, str)  # id, name, color
        self.treeview = Gtk.TreeView(model=self.store)
        self.treeview.set_reorderable(True)
        
        # Колонка с иконкой и именем
        renderer_pixbuf = Gtk.CellRendererPixbuf()
        renderer_text = Gtk.CellRendererText()
        column = Gtk.TreeViewColumn("Tag")
        column.pack_start(renderer_pixbuf, False)
        column.pack_start(renderer_text, True)
        column.set_cell_data_func(renderer_pixbuf, self._render_icon)
        column.add_attribute(renderer_text, "text", 1)
        self.treeview.append_column(column)
        
        # Заполняем список тегами
        tags = self.manager.get_tags()
        for tag in tags:
            self.store.append([tag["id"], tag["name"], tag["color"]])
        
        scrolled.add(self.treeview)
        vbox.pack_start(scrolled, True, True, 0)
        
        self.show_all()
    
    def _render_icon(self, _column, cell, model, iter, _data):
        """Рендерит иконку тега в TreeView"""
        color = model.get_value(iter, 2)
        icon_path = TagIconGenerator.create_tag_icon(color, 24)
        
        if os.path.exists(icon_path):
            try:
                pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_scale(icon_path, 24, 24, True)
                cell.set_property("pixbuf", pixbuf)
            except Exception as e:
                print(f"Error loading icon: {e}")
                cell.set_property("pixbuf", None)
        else:
            cell.set_property("pixbuf", None)
    
    def get_ordered_tag_ids(self):
        """Возвращает список ID тегов в текущем порядке"""
        tag_ids = []
        iter = self.store.get_iter_first()
        while iter:
            tag_id = self.store.get_value(iter, 0)
            tag_ids.append(tag_id)
            iter = self.store.iter_next(iter)

        return tag_ids


class TagLocationWidget(Gtk.Box):
    def __init__(self, manager: TagManager, extension):
        Gtk.Box.__init__(self, orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.manager = manager
        self.extension = extension
        self.drag_source_button = None
        self.drag_source_tag_id = None

        self.main_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=0)

        label_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=4)
        label_box.set_margin_start(12)
        label_box.set_margin_end(8)
        label_box.set_margin_top(4)
        label_box.set_margin_bottom(4)

        tag_icon = Gtk.Image.new_from_icon_name("tag", Gtk.IconSize.MENU)
        label_box.pack_start(tag_icon, False, False, 0)

        label = Gtk.Label(label="Tags:")
        label_box.pack_start(label, False, False, 0)
        
        self.main_box.pack_start(label_box, False, False, 0)

        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.NEVER)
        scrolled.set_size_request(-1, 40)

        self.tags_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=4)
        self.tags_box.set_margin_top(4)
        self.tags_box.set_margin_bottom(4)
        self.tags_box.set_margin_start(4)
        self.tags_box.set_margin_end(8)

        scrolled.add(self.tags_box)
        self.main_box.pack_start(scrolled, True, True, 0)
        
        # Кнопка настроек
        self.settings_button = Gtk.Button()
        self.settings_button.set_relief(Gtk.ReliefStyle.NONE)
        self.settings_button.set_tooltip_text("Reorder tags")
        
        settings_icon = Gtk.Image.new_from_icon_name("view-continuous-symbolic", Gtk.IconSize.BUTTON)
        self.settings_button.set_image(settings_icon)
        
        self.settings_button.set_margin_end(8)
        self.settings_button.set_margin_top(4)
        self.settings_button.set_margin_bottom(4)
        self.settings_button.connect("clicked", self.on_settings_clicked)
        
        self.main_box.pack_end(self.settings_button, False, False, 0)

        self.pack_start(self.main_box, True, True, 0)

        self.separator = Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL)
        self.pack_start(self.separator, False, False, 0)

        self.refresh()
    
    def on_settings_clicked(self, button):
        """Обработчик нажатия на кнопку настроек"""
        # Получаем родительское окно
        parent_window = self.get_toplevel()
        if not isinstance(parent_window, Gtk.Window):
            parent_window = None
        
        dialog = TagReorderDialog(parent_window, self.manager)
        response = dialog.run()
        
        if response == Gtk.ResponseType.OK:
            # Получаем новый порядок тегов
            new_order = dialog.get_ordered_tag_ids()
            
            # Сохраняем новый порядок
            if self.manager.reorder_tags(new_order):
                # Обновляем все виджеты
                for widget in self.extension.location_widgets:
                    widget.refresh()
        
        dialog.destroy()

    def refresh(self):
        for child in self.tags_box.get_children():
            self.tags_box.remove(child)

        tags = self.manager.get_tags()
        
        # Скрываем виджет если тегов нет
        if not tags:
            self.hide()
            return
        else:
            self.show_all()
            
        # Скрываем кнопку настроек если тегов меньше 2
        if len(tags) < 2:
            self.settings_button.hide()
        else:
            self.settings_button.show()

        for tag in tags:
            # EventBox для обработки событий
            event_box = Gtk.EventBox()
            
            btn = Gtk.Button(label=tag["name"])
            btn.set_relief(Gtk.ReliefStyle.NONE)

            # Увеличенный размер иконки тега (24px вместо 16px)
            icon_path = TagIconGenerator.create_tag_icon(tag["color"], 24)
            if os.path.exists(icon_path):
                pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_scale(
                    icon_path, 24, 24, True
                )
                image = Gtk.Image.new_from_pixbuf(pixbuf)
                btn.set_image(image)
                btn.set_always_show_image(True)

            btn.connect("clicked", self.on_tag_clicked, tag["id"], tag["name"])
            btn.connect(
                "button-press-event", self.on_tag_button_press, tag["id"], tag["name"], tag["color"]
            )

            event_box.add(btn)
            
            # Настройка Drag & Drop для EventBox
            event_box.drag_source_set(
                Gdk.ModifierType.BUTTON1_MASK,
                [],
                Gdk.DragAction.MOVE
            )
            event_box.drag_source_add_text_targets()
            event_box.connect("drag-begin", self.on_drag_begin, tag["id"])
            event_box.connect("drag-data-get", self.on_drag_data_get, tag["id"])
            event_box.connect("drag-end", self.on_drag_end)

            event_box.drag_dest_set(
                Gtk.DestDefaults.ALL,
                [],
                Gdk.DragAction.MOVE
            )
            event_box.drag_dest_add_text_targets()
            event_box.connect("drag-data-received", self.on_drag_data_received, tag["id"])
            event_box.connect("drag-motion", self.on_drag_motion)

            self.tags_box.pack_start(event_box, False, False, 0)
            event_box.show_all()

    def on_drag_begin(self, _widget, _context, tag_id):
        self.drag_source_tag_id = tag_id

    def on_drag_data_get(self, _widget, _context, data, _info, _time, tag_id):
        data.set_text(str(tag_id), -1)

    def on_drag_end(self, _widget, _context):
        self.drag_source_tag_id = None

    def on_drag_motion(self, _widget, context, _x, _y, time):
        Gdk.drag_status(context, Gdk.DragAction.MOVE, time)
        return True

    def on_drag_data_received(self, _widget, context, _x, _y, data, _info, time, target_tag_id):
        source_tag_id = data.get_text()
        if not source_tag_id or source_tag_id == target_tag_id:
            context.finish(False, False, time)
            return

        # Получаем текущий порядок тегов
        tags = self.manager.get_tags()
        tag_ids = [tag["id"] for tag in tags]
        
        if source_tag_id not in tag_ids or target_tag_id not in tag_ids:
            context.finish(False, False, time)
            return

        # Перемещаем тег
        source_index = tag_ids.index(source_tag_id)
        target_index = tag_ids.index(target_tag_id)
        
        tag_ids.pop(source_index)
        tag_ids.insert(target_index, source_tag_id)

        # Обновляем порядок в базе данных
        self.manager.reorder_tags(tag_ids)
        
        context.finish(True, False, time)
        
        # Обновляем все виджеты
        for w in self.extension.location_widgets:
            w.refresh()

    def _get_file_info_for_path(self, file_path):
        """Получаем Nemo.FileInfo для файла по пути"""
        try:
            from gi.repository import Gio
            file = Gio.File.new_for_path(file_path)
            return Nemo.FileInfo.create_for_uri(file.get_uri())
        except Exception as e:
            print(f"Error getting file info for {file_path}: {e}")
            return None

    def _delayed_invalidate_path(self, file_path):
        """Отложенная инвалидация файла для обновления эмблемы"""
        file_info = self._get_file_info_for_path(file_path)
        if file_info:
            file_info.invalidate_extension_info()
        return False  # Возвращаем False, чтобы timeout выполнился только один раз

    def on_tag_button_press(self, _button, event, tag_id, tag_name, tag_color):
        if event.button == 3:  # ПКМ
            menu = Gtk.Menu()

            # Пункт "Rename tag"
            rename_item = Gtk.MenuItem(label="Rename tag")
            rename_item.connect("activate", self.on_rename_tag, tag_id, tag_name, tag_color)
            menu.append(rename_item)

            # Пункт "Change color"
            color_item = Gtk.MenuItem(label="Change color")
            color_item.connect("activate", self.on_change_color, tag_id, tag_name, tag_color)
            menu.append(color_item)

            # Разделитель
            separator = Gtk.SeparatorMenuItem()
            menu.append(separator)

            # Пункт "Delete tag"
            delete_item = Gtk.MenuItem(label="Delete tag")
            delete_item.connect("activate", self.on_delete_tag, tag_id, tag_name, tag_color)
            menu.append(delete_item)

            menu.show_all()
            menu.popup(None, None, None, None, event.button, event.time)
            return True
        return False

    def on_rename_tag(self, _menu_item, tag_id, tag_name, tag_color):
        dialog = RenameTagDialog(initial_name=tag_name)
        new_name = dialog.run()
        
        if new_name and new_name != tag_name:
            self.manager.update_tag(tag_id, new_name, tag_color)
            
            # Обновляем все виджеты
            for widget in self.extension.location_widgets:
                widget.refresh()

    def on_change_color(self, _menu_item, tag_id, tag_name, tag_color):
        dialog = ColorPickerDialog(initial_color=tag_color)
        new_color = dialog.run_dialog()
        
        if new_color and new_color != tag_color:
            # Получаем все файлы с этим тегом ДО обновления
            files = self.manager.get_files_by_tag(tag_id)
            
            # Обновляем цвет тега
            self.manager.update_tag(tag_id, tag_name, new_color)
            
            # Создаём новую иконку для тега
            TagIconGenerator.create_tag_icon(new_color, 24)
            
            # Пересоздаём эмблемы для всех файлов с этим тегом
            for file_path in files:
                if not os.path.exists(file_path):
                    continue
                    
                # Получаем обновлённый список тегов для файла
                tags_for_file = self.manager.get_tags_for_file(file_path)
                colors = [t["color"] for t in tags_for_file if "color" in t]
                
                # Генерируем новую эмблему с обновлённым цветом
                if colors:
                    TagIconGenerator.create_emblem_icon(colors)
                
                # Асинхронная задержка 1000мс перед инвалидацией
                GLib.timeout_add(1000, self._delayed_invalidate_path, file_path)
            
            # Обновляем тему иконок GTK
            GLib.timeout_add(1000, self.extension.refresh_all_visible_files)
            
            # Обновляем все виджеты
            for widget in self.extension.location_widgets:
                widget.refresh()

    def on_delete_tag(self, _menu_item, tag_id, tag_name, tag_color):
        dialog = Gtk.MessageDialog(
            transient_for=None,
            flags=0,
            message_type=Gtk.MessageType.QUESTION,
            buttons=Gtk.ButtonsType.YES_NO,
            text=f"Delete tag '{tag_name}'?",
        )
        dialog.format_secondary_text(
            "This will remove the tag from all files. This action cannot be undone."
        )

        response = dialog.run()
        dialog.destroy()

        if response == Gtk.ResponseType.YES:
            # Получаем все файлы с этим тегом ДО удаления
            files = self.manager.get_files_by_tag(tag_id)
            
            # Удаляем тег
            self.manager.delete_tag(tag_id)
            
            # Удаляем все эмблемы, содержащие этот цвет
            TagIconGenerator.delete_emblems_with_color(tag_color)
            
            # Пересоздаём эмблемы для всех файлов, которые имели этот тег
            for file_path in files:
                if not os.path.exists(file_path):
                    continue
                    
                # Получаем оставшиеся теги для файла
                tags_for_file = self.manager.get_tags_for_file(file_path)
                colors = [t["color"] for t in tags_for_file if "color" in t]
                
                # Генерируем эмблему для оставшихся тегов (или удаляем если тегов не осталось)
                if colors:
                    TagIconGenerator.create_emblem_icon(colors)
                
                # Асинхронная задержка 1000мс перед инвалидацией
                GLib.timeout_add(1000, self._delayed_invalidate_path, file_path)
            
            # Обновляем тему иконок GTK
            GLib.timeout_add(1000, self.extension.refresh_all_visible_files)
            
            # Обновляем все виджеты
            for widget in self.extension.location_widgets:
                widget.refresh()

    def on_tag_clicked(self, _button, tag_id, tag_name):
        view_dir = self.manager.create_tag_view(tag_id, tag_name)
        if view_dir:
            try:
                subprocess.Popen(["nemo", "--existing-window", "-t", view_dir])
            except Exception as e:
                print(f"Error opening Nemo tab: {e}")

                # Fallback: открываем новое окно
                try:
                    subprocess.Popen(["nemo", view_dir])
                except Exception as e2:
                    print(f"Error opening Nemo window: {e2}")
        else:
            dialog = Gtk.MessageDialog(
                transient_for=None,
                flags=0,
                message_type=Gtk.MessageType.INFO,
                buttons=Gtk.ButtonsType.OK,
                text=f"No files tagged with '{tag_name}'",
            )
            dialog.run()
            dialog.destroy()

# ===== END src/ui.py =====

# ===== BEGIN src/extension.py =====

import os
from pathlib import Path
from typing import List

from gi.repository import GLib
from gi.repository import GObject
from gi.repository import Nemo



class NemoTagsExtension(
    GObject.GObject, Nemo.MenuProvider, Nemo.LocationWidgetProvider, Nemo.InfoProvider
):
    def __init__(self):
        GObject.Object.__init__(self)
        self.manager = TagManager()
        self.icon_generator = TagIconGenerator()
        self.location_widgets = []
        tags = self.manager.get_tags()

        # Предварительно генерируем все возможные комбинации эмблем.
        # Обновление кеша и темы иконок произойдёт внутри TagIconGenerator
        # после завершения фоновой генерации.
        TagIconGenerator.pregenerate_all_combinations(tags)
        self.refresh_all_visible_files()

    def refresh_all_visible_files(self):
        TagIconGenerator.refresh_icon_theme()
        for widget in self.location_widgets:
            widget.refresh()

    def _get_tag_id_from_view_path(self, path: str) -> str:
        """Извлекает tag_id из пути к файлу в виртуальной папке тега"""
        path_obj = Path(path)
        # Проверяем, находится ли файл в директории views
        try:
            relative = path_obj.relative_to(VIEWS_DIR)
            # Первая часть пути - это папка tag-{id}
            tag_folder = relative.parts[0]
            if tag_folder.startswith("tag-"):
                return tag_folder[4:]  # Убираем префикс "tag-"
        except (ValueError, IndexError):
            pass
        return None

    def update_file_info(self, file_info: Nemo.FileInfo):
        if file_info.get_uri_scheme() != "file":
            return Nemo.OperationResult.COMPLETE
        path = file_info.get_location().get_path()
        if not path:
            return Nemo.OperationResult.COMPLETE

        # Проверяем, находится ли файл в виртуальной папке тега
        tag_id = self._get_tag_id_from_view_path(path)

        if tag_id:
            # Файл находится в виртуальной папке тега
            tag = self.manager.get_tag_by_id(tag_id)
            if tag and "color" in tag:
                emblem_name = TagIconGenerator.create_emblem_icon([tag["color"]])
                file_info.add_emblem(emblem_name)
        else:
            # Обычный файл - показываем все его теги
            tags = self.manager.get_tags_for_file(path)
            if tags:
                colors = [tag["color"] for tag in tags if "color" in tag]
                if colors:
                    emblem_name = TagIconGenerator.create_emblem_icon(colors)
                    file_info.add_emblem(emblem_name)

        return Nemo.OperationResult.COMPLETE

    def get_file_items(self, window, files: List[Nemo.FileInfo]):
        menu_items = []
        target_files = [f for f in files if f.get_uri_scheme() == "file"]

        if not target_files:
            return []

        single_file = len(target_files) == 1
        file_path = target_files[0].get_location().get_path() if single_file else None
        submenu = Nemo.Menu()
        tags = self.manager.get_tags()

        if tags:
            for tag in tags:
                is_assigned = False
                if single_file and file_path:
                    is_assigned = self.manager.is_file_tagged(tag["id"], file_path)
                label = f"✓ {tag['name']}" if is_assigned else tag["name"]
                tag_item = Nemo.MenuItem(
                    name=f"NemoTags::toggle_tag_{tag['id']}",
                    label=label,
                )
                icon_path = self.icon_generator.create_tag_icon(tag["color"], 16)
                if os.path.exists(icon_path):
                    tag_item.set_property("icon", icon_path)
                if is_assigned:
                    tag_item.connect(
                        "activate", self.remove_tag, target_files, tag["id"]
                    )
                else:
                    tag_item.connect(
                        "activate", self.apply_tag, target_files, tag["id"]
                    )
                submenu.append_item(tag_item)
        if tags:
            sep = Nemo.MenuItem(
                name="NemoTags::separator", label="─────────", sensitive=False
            )
            submenu.append_item(sep)
        create_item = Nemo.MenuItem(
            name="NemoTags::create_tag",
            label="Create Tag",
            icon="list-add",
        )
        create_item.connect("activate", self.create_tag, target_files, window)
        submenu.append_item(create_item)
        tags_menu = Nemo.MenuItem(
            name="NemoTags::assign_tag_main",
            label="Assign Tag",
            icon="tag",
        )
        tags_menu.set_submenu(submenu)
        menu_items.append(tags_menu)
        return menu_items

    def get_widget(self, _uri, _window):
        widget = TagLocationWidget(self.manager, self)
        self.location_widgets.append(widget)
        return widget

    def _delayed_invalidate(self, file_info):
        file_info.invalidate_extension_info()
        return False

    def apply_tag(self, _menu, files, tag_id: str):
        for f in files:
            loc = f.get_location()
            if loc is None:
                continue
            path = loc.get_path()
            if not path:
                continue
            self.manager.assign_tag_to_file(tag_id, path)
            tags_for_file = self.manager.get_tags_for_file(path)
            colors = [t["color"] for t in tags_for_file if "color" in t]
            if colors:
                TagIconGenerator.create_emblem_icon(colors)
            self.refresh_all_visible_files()
            GLib.timeout_add(1000, self._delayed_invalidate, f)

    def remove_tag(self, menu, files, tag_id: str):
        for f in files:
            loc = f.get_location()
            if loc is None:
                continue

            path = loc.get_path()
            if not path:
                continue

            self.manager.unassign_tag_from_file(tag_id, path)
            tags_for_file = self.manager.get_tags_for_file(path)
            colors = [t["color"] for t in tags_for_file if "color" in t]

            # Эмблема без удалённого цвета
            if colors:
                TagIconGenerator.create_emblem_icon(colors)
            
            # Если файл вообще остался без тегов, можно "удалить" эмблему путем инвалидации
            self.refresh_all_visible_files()
            GLib.timeout_add(1000, self._delayed_invalidate, f)

        # Теперь пересоздаём эмблемы для всех возможных комбинаций без удалённого цвета
        tags = self.manager.get_tags()
        colors = [tag["color"] for tag in tags if "color" in tag]
        TagIconGenerator.pregenerate_all_combinations(tags)
        self.refresh_all_visible_files()

    def create_tag(self, menu, files, window):
        dialog = TagDialog(parent=window)
        name, color = dialog.run_dialog()

        if not (name and color):
            return
        
        tag_id = self.manager.create_tag(name, color)
        tags = self.manager.get_tags()
        TagIconGenerator.create_tag_icon(color, 16)
        TagIconGenerator.pregenerate_all_combinations(tags)
        self.refresh_all_visible_files()
        for f in files:
            loc = f.get_location()

            if loc is None:
                continue

            path = loc.get_path()
            if not path:
                continue

            self.manager.assign_tag_to_file(tag_id, path)
            tags_for_file = self.manager.get_tags_for_file(path)
            colors = [t["color"] for t in tags_for_file if "color" in t]

            if colors:
                TagIconGenerator.create_emblem_icon(colors)

            GLib.timeout_add(1000, self._delayed_invalidate, f)

        for widget in self.location_widgets:
            widget.refresh()
            
        self.refresh_all_visible_files()

# ===== END src/extension.py =====

