from gi.repository import Nemo, GObject

class UpdateFileInfoAsync(GObject.GObject, Nemo.InfoProvider):
    def __init__(self):
        super(UpdateFileInfoAsync, self).__init__()
        pass

    def update_file_info_full(self, provider, handle, closure, file):
        print "update_file_info_full"
        gobject.timeout_add_seconds(3, self.update_cb, provider, handle, closure)
        return Nemo.OperationResult.IN_PROGRESS

    def update_cb(self, provider, handle, closure):
        print "update_cb"
        Nemo.info_provider_update_complete_invoke(closure, provider, handle, Nemo.OperationResult.FAILED)
