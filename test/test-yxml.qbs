import qbs

CppApplication {
    consoleApplication: true
    files: [
        "test-yxml.cpp",
        "yxml/yxml.h",
        "yxml/yxml.c",
    ]
    cpp.cxxLanguageVersion: "c++20"
    cpp.enableRtti: false
    cpp.includePaths: ["../source", "yxml"]
    cpp.staticLibraries: "expat"
    Properties {
        condition: qbs.buildVariant === "release"
        cpp.cxxFlags: ["-Os"]
    }
}
