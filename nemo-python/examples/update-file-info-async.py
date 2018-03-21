from gi.repository import Nemo, GObject

class UpdateFileInfoAsync(GObject.GObject, Nemo.InfoProvider):
    def __init__(self):
        self.timers = []
        pass

    def update_file_info_full(self, provider, handle, closure, file):
        print("update_file_info_full")
        self.timers.append(GObject.timeout_add_seconds(3, self.update_cb, provider, handle, closure))
        return Nemo.OperationResult.IN_PROGRESS

    def update_cb(self, provider, handle, closure):
        print("update_cb")
        Nemo.info_provider_update_complete_invoke(closure, provider, handle, Nemo.OperationResult.FAILED)

    def cancel_update(self, provider, handle):
        print("cancel_update")
        for t in self.timers:
            GObject.source_remove(t)
        self.timers = []
