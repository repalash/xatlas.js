mergeInto(LibraryManager.library, {
    onAtlasProgress: function(mode, progress) {
        if(Module.onAtlasProgress) Module.onAtlasProgress(mode, progress);
    }
});