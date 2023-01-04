import qbs

CppApplication {
    consoleApplication: true
    files: [
        "test-expat.cpp",
        "tools.hpp",
    ]
    cpp.cxxLanguageVersion: "c++20"
    cpp.enableRtti: false
    cpp.includePaths: ["../source"]
    cpp.staticLibraries: "expat"
    Properties {
        condition: qbs.buildVariant === "release"
        cpp.cxxFlags: ["-Ofast"]
    }
}
