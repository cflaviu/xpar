import qbs

CppApplication {
    consoleApplication: true
    files: [
        "test-xpar.cpp",
        "tools.hpp",
    ]
    cpp.cxxLanguageVersion: "c++11"
    cpp.enableRtti: false
    cpp.includePaths: ["../source"]

    Properties {
        condition: qbs.buildVariant === "release"
        cpp.cxxFlags: ["-Os"]
    }
    Properties {
        condition: qbs.buildVariant === "debug"
        cpp.defines: ["ASAN_OPTIONS=abort_on_error=1:report_objects=1:sleep_before_dying=1"]
        cpp.cxxFlags: "-fsanitize=address"
        cpp.staticLibraries: "asan"
    }
}
