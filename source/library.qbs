import qbs

StaticLibrary {
    name: "xpar"
    Depends {
        name: "cpp"
    }
    files: [
        "xpar.hpp",
    ]
    cpp.cxxLanguageVersion: "c++14"
    cpp.enableRtti: false
    cpp.includePaths: ["."]
}

