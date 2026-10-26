#include <romfs/romfs.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <map>
#include <mutex>
#include <stdexcept>

const std::map<fs::path, romfs::Resource>& ROMFS_CONCAT(ROMFS_NAME, _get_resources)();
const std::vector<fs::path>& ROMFS_CONCAT(ROMFS_NAME, _get_paths)();
const std::string& ROMFS_CONCAT(ROMFS_NAME, _get_name)();

namespace {

    // Loose files before the embedded ones (opt-in, for development): when
    // AURORE_RESOURCES_DIR names a directory laid out like the resources
    // folder, a file found there is used instead of the embedded copy, so XML,
    // images or translations can be changed without rebuilding the app (a
    // restart is enough). A file is read once and kept: callers hold on to the
    // Resource references, and std::map nodes do not move.
    struct Overlay {
        std::mutex mutex;
        std::map<fs::path, std::string> contents;
        std::map<fs::path, romfs::Resource> resources;
    };

    Overlay &overlay() {
        static Overlay instance;
        return instance;
    }

    const fs::path *overlayRoot() {
        static const fs::path root = [] {
            const char *dir = std::getenv("AURORE_RESOURCES_DIR");
            return dir != nullptr ? fs::path(dir) : fs::path();
        }();
        return root.empty() ? nullptr : &root;
    }

    const romfs::Resource *overlayGet(const fs::path &path) {
        const fs::path *root = overlayRoot();
        if (root == nullptr)
            return nullptr;

        Overlay &store = overlay();
        std::lock_guard<std::mutex> lock(store.mutex);

        auto cached = store.resources.find(path);
        if (cached != store.resources.end())
            return &cached->second;

        std::ifstream file((*root / path).string(), std::ios::binary);
        if (!file)
            return nullptr;
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        // An empty file is more likely a broken copy than a wanted change
        if (content.empty())
            return nullptr;

        // Like the generated arrays: a 0 after the bytes, outside the size,
        // for Resource::string()
        content.push_back('\0');
        std::string &stored = store.contents.emplace(path, std::move(content)).first->second;
        auto bytes = nonstd::span<std::byte>(reinterpret_cast<std::byte *>(stored.data()), stored.size() - 1);
        return &store.resources.emplace(path, romfs::Resource(bytes)).first->second;
    }

    void overlayList(const fs::path &parent, std::vector<fs::path> &result) {
        const fs::path *root = overlayRoot();
        if (root == nullptr)
            return;

#ifdef USE_BOOST_FILESYSTEM
        using error_code = boost::system::error_code;
#else
        using error_code = std::error_code;
#endif
        error_code error;
        const fs::path dir = *root / parent;
        if (!fs::is_directory(dir, error))
            return;

        auto add = [&](const fs::path &file) {
            // get() skips empty files: do not list one it could not give
            error_code fileError;
            if (!fs::is_regular_file(file, fileError) || fs::file_size(file, fileError) == 0 || fileError)
                return;
            fs::path relative = parent.empty() ? file.lexically_relative(*root) : parent / file.filename();
            if (std::find(result.begin(), result.end(), relative) == result.end())
                result.push_back(relative);
        };
        if (parent.empty()) {
            for (fs::recursive_directory_iterator it(dir, error), end; !error && it != end; it.increment(error))
                add(it->path());
        } else {
            for (fs::directory_iterator it(dir, error), end; !error && it != end; it.increment(error))
                add(it->path());
        }
    }

}

namespace romfs {

    const romfs::Resource &impl::ROMFS_CONCAT(get_, LIBROMFS_PROJECT_NAME)(const fs::path &path) {
        if (const romfs::Resource *loose = overlayGet(path))
            return *loose;

        try {
            return ROMFS_CONCAT(ROMFS_NAME, _get_resources)().at(path);
        } catch (std::out_of_range &ignored) {
            throw std::invalid_argument(std::string("Invalid romfs resource path for '" + romfs::name() + "' : ") + path.string());
        }
    }

    std::vector<fs::path> impl::ROMFS_CONCAT(list_, LIBROMFS_PROJECT_NAME)(const fs::path &parent) {
        std::vector<fs::path> result;
        if (parent.empty()) {
            result = ROMFS_CONCAT(ROMFS_NAME, _get_paths)();
        } else {
            for (const auto &p : ROMFS_CONCAT(ROMFS_NAME, _get_paths)())
                if (p.parent_path() == parent)
                    result.push_back(p);
        }
        overlayList(parent, result);
        return result;
    }

    const std::string &impl::ROMFS_CONCAT(name_, LIBROMFS_PROJECT_NAME)() {
        return ROMFS_CONCAT(ROMFS_NAME, _get_name)();
    }

}
