import urllib
import locale, gettext, os

from gi.repository import GObject
from gi.repository import Gio
from gi.repository import Gtk

from gi.repository import Nemo

METADATA_EMBLEMS = 'metadata::emblems'

GUI = """
<interface>
  <requires lib="gtk+" version="3.0"/>
  <object class="GtkScrolledWindow" id="mainWindow">
    <property name="visible">True</property>
    <property name="can_focus">True</property>
    <property name="hscrollbar_policy">never</property>
    <child>
      <object class="GtkViewport" id="viewport1">
        <property name="visible">True</property>
        <property name="can_focus">False</property>
        <child>
          <object class="GtkGrid" id="grid">
            <property name="visible">True</property>
            <property name="can_focus">False</property>
            <property name="margin_left">4</property>
            <property name="margin_right">4</property>
            <property name="margin_top">4</property>
            <property name="margin_bottom">4</property>
            <property name="vexpand">True</property>
            <property name="row_spacing">4</property>
            <property name="column_spacing">4</property>
            <property name="row_homogeneous">True</property>
            <property name="column_homogeneous">True</property>
          </object>
        </child>
      </object>
    </child>
  </object>
</interface>"""

HIDE_EMBLEMS = ['emblem-desktop', 'emblem-noread', 'emblem-nowrite', 'emblem-readonly', 'emblem-shared', 'emblem-synchronizing', 'emblem-symbolic-link', 'emblem-unreadable']

class EmblemPropertyPage(GObject.GObject, Nemo.PropertyPageProvider):
    
    def __init__(self):
        self.default_icon_theme = Gtk.IconTheme.get_default()
        self.display_names = {}
        icon_names = self.default_icon_theme.list_icons()
        for icon_name in icon_names:
            if not icon_name in HIDE_EMBLEMS and icon_name.startswith('emblem-') and not icon_name.endswith('-symbolic'):
                # icon_name is emblem
                icon_info = self.default_icon_theme.lookup_icon(icon_name, 32, 0)
                display_name = icon_info.get_display_name()
                
                if not display_name:
                    display_name = icon_name[7].upper() + icon_name[8:]
                
                self.display_names[icon_name] = display_name
    
    def get_property_pages(self, files):
        # files: list of NemoVFSFile
        if len(files) != 1:
            return
        
        file = files[0]
        if file.get_uri_scheme() != 'file':
            return
        
        self.filename = urllib.unquote(file.get_uri()[7:])
        self.gio_file = Gio.File.new_for_path(self.filename)
        self.file_info = self.gio_file.query_info(METADATA_EMBLEMS, 0, None)
        self.file_emblem_names = self.file_info.get_attribute_stringv(METADATA_EMBLEMS)
        
        #GUI
        locale.setlocale(locale.LC_ALL, '')
        gettext.bindtextdomain('nemo-emblems')
        gettext.textdomain('nemo-emblems')
        _ = gettext.gettext
        self.property_label = Gtk.Label(_('Emblems'))
        self.property_label.show()
        
        self.builder = Gtk.Builder()
        self.builder.add_from_string(GUI)
        
        #connect signals to python methods
        self.builder.connect_signals(self)
        
        #connect gtk objects to python variables
        for obj in self.builder.get_objects():
            if issubclass(type(obj), Gtk.Buildable):
                name = Gtk.Buildable.get_name(obj)
                setattr(self, name, obj)
        
        left = 0
        top = 0
        for emblem_name, display_name in sorted(self.display_names.items(), key=lambda x: x[1]):
            checkbutton = Gtk.CheckButton()
            checkbutton.set_label(display_name)
            
            image = Gtk.Image.new_from_icon_name(emblem_name, Gtk.IconSize.BUTTON)
            image.set_pixel_size(24) # this should not be necessary            
            checkbutton.set_always_show_image(True)
            checkbutton.set_image(image)            
            
            if emblem_name in self.file_emblem_names:
                checkbutton.set_active(True)
            
            checkbutton.connect("toggled", self.on_button_toggled, emblem_name)
            checkbutton.show()
            
            self.grid.attach(checkbutton, left, top, 1, 1)
            left += 1
            if left > 2:
                left = 0
                top += 1
        
        return Nemo.PropertyPage(name="NemoPython::emblem", label=self.property_label, page=self.mainWindow),
    
    def on_button_toggled(self, button, emblem_name):
        if button.get_active() and not emblem_name in self.file_emblem_names:
            self.file_emblem_names.append(emblem_name)
        else:
            self.file_emblem_names.remove(emblem_name)
        
        emblems = list(self.file_emblem_names)
        emblems.append(None)
        
        self.file_info.set_attribute_stringv(METADATA_EMBLEMS, emblems)
        self.gio_file.set_attributes_from_info(self.file_info, 0, None)
        os.system("touch \"%s\"" % self.filename) # touch the file (to force Nemo to re-render its icon)
