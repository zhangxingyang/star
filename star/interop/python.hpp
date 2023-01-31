#ifndef STAR_PYTHON_HPP
#define STAR_PYTHON_HPP

#include <Python.h>

#include <exception>
#include <filesystem>

class Python {
public:
    inline Python() {
        Py_Initialize();
        this->check_initialized();
    }

    explicit inline Python(PyConfig &config) {
        // 支持自定义config
        this->config = config;
        Py_InitializeFromConfig(&this->config);
        this->check_initialized();
    }

    template<typename ...Paths>
    inline explicit Python(const char *python_home, const Paths ...paths) {
        /*
         * paths:
         *   python home         -> config.home                 -- python installation directory
         *   python dlls         -> config.module_search_paths  -- import packages need
         *   python lib          -> config.module_search_paths  -- import packages need
         *   module search path  -> config.module_search_paths  -- import your own packages
         */

        PyConfig_InitPythonConfig(&this->config);
        this->config.home = Py_DecodeLocale(python_home, nullptr);
        if constexpr (sizeof...(paths) > 0) {
            this->config.module_search_paths_set = 1;
        }
        auto set_paths = [this](auto &path) {
            PyWideStringList_Append(&this->config.module_search_paths, Py_DecodeLocale(path, nullptr));
        };
        (set_paths(paths), ...);
        Py_InitializeFromConfig(&this->config);
        this->check_initialized();
    }

    class Object {
    public:
        Object() = default;

        Object(PyObject *obj) { // NOLINT(google-explicit-constructor)
            this->object = obj;
        }

        Object &operator=(PyObject *obj) {
            this->object = obj;
            return *this;
        }

        PyObject *operator*() {
            return this->object;
        }

        PyObject *get() {
            return this->object;
        }

         ~Object() {
            Py_XDECREF(this->object);
        }

    private:
        PyObject *object{};
    };

    inline bool is_initialized() { // NOLINT(readability-convert-member-functions-to-static)
        return Py_IsInitialized();
    }

    inline PyObject *get_attr_string(PyObject *py_module, const char *attr_name) { // NOLINT(readability-convert-member-functions-to-static)
        return PyObject_GetAttrString(py_module, attr_name);
    }

    inline bool is_tuple(PyObject *obj) { // NOLINT(readability-convert-member-functions-to-static)
        return PyTuple_Check(obj);
    }

    inline Py_ssize_t get_tuple_size(PyObject *tuple) { // NOLINT(readability-convert-member-functions-to-static)
        return PyTuple_Size(tuple);
    }

    template<typename ...Args>
    inline PyObject *build_value(const char *format, Args ...args) {
        return Py_BuildValue(format, args...);
    }

    inline bool execute(const char *str) { // NOLINT(readability-convert-member-functions-to-static)
        // 执行单独python代码
        int result = PyRun_SimpleString(str);
        return result == 0;
    }

    inline void
    run_simple_file(const std::filesystem::path &path) { // NOLINT(readability-convert-member-functions-to-static)
        if (!exists(path)) {
            throw std::exception("run file path does not exist");
        }
        FILE *file = fopen(path.string().c_str(), "r");
        PyRun_SimpleFile(file, path.filename().string().c_str());
    }

    inline const char *
    get_utf8_from_PyUnicode(PyObject *unicode) { // NOLINT(readability-convert-member-functions-to-static)
        return PyUnicode_AsUTF8(unicode);
    }

    inline PyObject *
    get_PyUnicode_from_string(const char *str) { // NOLINT(readability-convert-member-functions-to-static)
        return PyUnicode_FromString(str);
    }

    inline PyObject *import_module(const char *name) { // NOLINT(readability-convert-member-functions-to-static)
        return PyImport_ImportModule(name);
    }

    template<typename ...Args>
    inline PyObject *call_function(PyObject *func, const char *format = nullptr, Args ...args) {
        return PyObject_CallFunction(func, format, args...);
    }

    inline PyObject *call_object(PyObject * py_object, PyObject * py_args = nullptr) { // NOLINT(readability-convert-member-functions-to-static)
        return PyObject_CallObject(py_object, py_args);
    }

    template<typename ...Args>
    inline PyObject *call_method(PyObject * object, const char * method_name, const char * format = nullptr, Args ...args) {
        return PyObject_CallMethod(object, method_name, format, args...);
    }

    template<typename ...Args>
    inline void reduce_reference_count(Args ...args) { // NOLINT(readability-convert-member-functions-to-static)
        auto reduce = [](auto &arg) {
            Py_XDECREF(arg);
        };
        (reduce(args), ...);
    }

    inline void print_error() { // NOLINT(readability-convert-member-functions-to-static)
        PyErr_Print();
    }

    ~Python() {
        PyConfig_Clear(&this->config);
        Py_Finalize();
    }

protected:
    inline void check_initialized() { // NOLINT(readability-convert-member-functions-to-static)
        if (!Py_IsInitialized()) {
            throw std::exception("Python initialize error");
        }
    }

private:
    PyConfig config{};
};

#endif //STAR_PYTHON_HPP

