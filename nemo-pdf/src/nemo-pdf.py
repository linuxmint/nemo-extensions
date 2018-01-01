import os,threading,gettext,urllib,locale
from gi.repository import Nemo, GObject, Gtk

locale.setlocale(locale.LC_ALL, '')
gettext.bindtextdomain("nemo-extensions")
gettext.textdomain("nemo-extensions")
_ = gettext.gettext

def all_files_are_pdf(files):
	for f in files:
		if not f.get_uri().endswith('.pdf'):
			return False
	return True

def all_threads_are_dead(threads):
	for t in threads:
		if t.isAlive():
			return False
	return True

def NemoPdfCommand(command, link):
	os.system(command)
	if link != None:
		os.rename(link + '_t', link)

class NemoPdf(Nemo.MenuProvider, GObject.GObject, Nemo.NameAndDescProvider):
	
	def __init__(self):
		print 'Initializing nemo-pdf'
	
	
	def pdftk_is_installed(self, first):
		if os.path.isfile('/usr/bin/pdftk'):
			return True
		
		confirm = Gtk.MessageDialog(None, 0, Gtk.MessageType.QUESTION, 
						Gtk.ButtonsType.YES_NO,
						_('Installation of PDF Toolkit necessary'))
		if first:
			confirm.format_secondary_text(_('Do you want to install PDF toolkit? Your operations will be performed after the installation.'))
		else:
			confirm.format_secondary_text(_('Installation failed. Do you want to try again? Your operations will be performed after the installation.'))
		response = confirm.run()
		confirm.destroy()
		
		if response == Gtk.ResponseType.YES:
			os.system('gksu apt-get install pdftk')
			return self.pdftk_is_installed(0)
		return False
	
	def __update_progress(self, bar):
		bar.pulse()
		return True
		
	def reduce(self, menu, files):
		win = Gtk.Window(title=_('Reducing PDF'))
		bar = Gtk.ProgressBar()
		win.add(bar)
		win.set_border_width(10)
		bar.set_text(_('Your PDF are reducing... Don\'t manipulate selected files!'))
		bar.set_show_text(True)
		win.show_all()
		
		t_list = []
		for f in files:
			link = urllib.unquote(f.get_uri()[7:])
			t = threading.Thread(target = lambda: NemoPdfCommand('gs -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -sOutputFile="' + link + '_t" "' + link + '"', link))
			t.start()
			t_list.append(t)
		
		id_time = GObject.timeout_add(50, self.__update_progress, bar)
		while not all_threads_are_dead(t_list):
			while Gtk.events_pending():
				Gtk.main_iteration()
		GObject.source_remove(id_time)
		win.destroy()
	
	def rotate_left(self, menu, files):
		if self.pdftk_is_installed(1):
			for f in files:
				link = urllib.unquote(f.get_uri()[7:])
				os.system('pdftk "' + link + '" cat 1-endleft output "' + link + '_t"')
				os.rename(link + '_t', link)
	
	def rotate_right(self, menu, files):
		if self.pdftk_is_installed(1):
			for f in files:
				link = urllib.unquote(f.get_uri()[7:])
				os.system('pdftk "' + link + '" cat 1-endright output "' + link + '_t"')
				os.rename(link + '_t', link)
	
	def flip(self, menu, files):
		if self.pdftk_is_installed(1):
			for f in files:
				link = urllib.unquote(f.get_uri()[7:])
				os.system('pdftk "' + link + '" cat 1-enddown output "' + link + '_t"')
				os.rename(link + '_t', link)
	
	def extract(self, menu, files):
		if self.pdftk_is_installed(1):
			f = urllib.unquote(files[0].get_uri()[7:])
			
			win = Gtk.Dialog(_('Extract PDF'), None, 0, 
						(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
						Gtk.STOCK_OK, Gtk.ResponseType.OK))
			
			e = Gtk.Entry()
			b1 = Gtk.RadioButton.new_with_label_from_widget(None, _('Extract following pages (example: 1-10 15-17):'))
			b2 = Gtk.RadioButton.new_with_label_from_widget(b1, _('Extract even pages'))
			b3 = Gtk.RadioButton.new_with_label_from_widget(b1, _('Extract odd pages'))
			
			b1.connect('toggled', lambda x, y: e.set_editable(True), None)
			b2.connect('toggled', lambda x, y: e.set_editable(False), None)
			b3.connect('toggled', lambda x, y: e.set_editable(False), None)
			
			vbox = win.get_content_area()
			vbox.pack_start(b1, True, True, 0)
			vbox.pack_start(e, True, True, 0)
			vbox.pack_start(b2, True, True, 0)
			vbox.pack_start(b3, True, True, 0)
			
			win.show_all()
			response = win.run()
			
			if response == Gtk.ResponseType.OK:
				dialogE = Gtk.FileChooserDialog(_('Save as...'),
                               None,
                               Gtk.FileChooserAction.SAVE,
                               (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                                Gtk.STOCK_SAVE, Gtk.ResponseType.OK))
				dialogE.set_default_response(Gtk.ResponseType.OK)
				dialogE.set_do_overwrite_confirmation(True)
				dialogE.set_current_name('*.pdf')
				response = dialogE.run()
			
				if response == Gtk.ResponseType.OK:
					if b1.get_active():
						os.system('pdftk "' + f + '" cat ' + e.get_text() + ' output "' + dialogE.get_filename() + '"')
					elif b2.get_active():
						os.system('pdftk "' + f + '" cat even output "' + dialogE.get_filename() + '"')
					else:
						os.system('pdftk "' + f + '" cat odd output "' + dialogE.get_filename() + '"')
				dialogE.destroy()
			win.destroy()
			
	
	def merge(self, menu, files):
		if self.pdftk_is_installed(1):
			dialogE = Gtk.FileChooserDialog('Save as...',
                               None,
                               Gtk.FileChooserAction.SAVE,
                               (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                                Gtk.STOCK_SAVE, Gtk.ResponseType.OK))
			dialogE.set_default_response(Gtk.ResponseType.OK)
			dialogE.set_do_overwrite_confirmation(True)
			dialogE.set_current_name('*.pdf')
			
			response = dialogE.run()
			file_s = dialogE.get_filename()
			dialogE.destroy()
			
			if response == Gtk.ResponseType.OK:
				l = []
				for f in files:
					l.append(urllib.unquote(f.get_uri()[7:]))
				t = threading.Thread(target = lambda: NemoPdfCommand('pdftk "' + '" "'.join(l) + '" cat output "' + file_s + '"', None))
				t.start()
				
				win = Gtk.Dialog('Merging PDF')
				bar = Gtk.ProgressBar()
				win.get_content_area().pack_start(bar, True, True, 0)
				bar.set_text('Your PDF are merging... Don\'t manipulate selected files!')
				bar.set_show_text(True)
				win.show_all()
				
				id_time = GObject.timeout_add(50, self.__update_progress, bar)
				while not all_threads_are_dead([t]):
					while Gtk.events_pending():
						Gtk.main_iteration()
				GObject.source_remove(id_time)
				win.destroy()
	
	def get_file_items(self, window, files):
		
		if len(files) == 0 or not all_files_are_pdf(files):
			return None
			
		top_menuitem = Nemo.MenuItem(name='NemoPdf::Menu',
							label=_('Pdf Tools'),
							tip=_('Tools for PDF files'),
							icon='gnome-mime-application-pdf')
		submenu = Nemo.Menu()
		top_menuitem.set_submenu(submenu)
        
		resize_mi = Nemo.MenuItem(name='NemoPdf::resize', 
			label=_('Reduce PDF size'), 
			tip=_('Reduce the size of your selected PDF files'),
			icon='')
		rotl_mi = Nemo.MenuItem(name='NemoPdf::rotl', 
			label=_('Rotate PDF to left'), 
			tip=_('Rotate all your PDF files to left'),
			icon='')
		rotr_mi = Nemo.MenuItem(name='NemoPdf::rotr', 
			label=_('Rotate PDF to right'), 
			tip=_('Rotate all your PDF files to right'),
			icon='')
		flip_mi = Nemo.MenuItem(name='NemoPdf::rotr', 
			label=_('Flip vertically PDF'), 
			tip=_('Flip vertically all your selected PDF files'),
			icon='')
			
		resize_mi.connect('activate', self.reduce, files)
		rotl_mi.connect('activate', self.rotate_left, files)
		rotr_mi.connect('activate', self.rotate_right, files)
		flip_mi.connect('activate', self.flip, files)
		
		submenu.append_item(resize_mi)
		submenu.append_item(rotl_mi)
		submenu.append_item(rotr_mi)
		submenu.append_item(flip_mi)
		
		if len(files) == 1:
			extr_mi = Nemo.MenuItem(name='NemoPdf::extr', 
				label=_('Extract PDF'), 
				tip=_('Extract parties from the selected PDF file'),
				icon='')
			extr_mi.connect('activate', self.extract, files)
			submenu.append_item(extr_mi)
		
		else:
			merge_mi = Nemo.MenuItem(name='NemoPdf::merg', 
				label=_('Merge PDF'), 
				tip=_('Merge all selected PDF files into one file'),
				icon='')
			merge_mi.connect('activate', self.merge, files)
			submenu.append_item(merge_mi)
			
		return top_menuitem,
		
	def get_background_items(self, window, file):
		pass
	
	
	def get_name_and_desc(self):
		return [('Nemo Pdf:::'+_('Tools to manage PDF files'))]
        
