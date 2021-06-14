#!/usr/bin/env python3

import os
import glob
import argparse

PROJECT_ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")


class Analyzer:
    def __init__(self, name: str, identifier: str):
        self.name = name
        self.identifier = identifier

    def getCanonicalName(self):
        return self.name.lower() + self.identifier

    def getDeclaration(self):
        return self.name + " " + self.getCanonicalName() + ";"


def listPrinter(file, _list: list, prefix=""):
    for line in _list:
        if isinstance(line, list):
            listPrinter(file, line, prefix + "\t")
        else:
            file.write(prefix + line + "\n")


targetpath = PROJECT_ROOT + "/{}/dispatchers/metaprogramming/{}{}"


def writeFiles(classname: str, header: list, cpp: list):
    for i in range(2):
        path = targetpath.format(("include" if i == 0 else "src"), classname, (".h" if i == 0 else ".cpp"))
        with open(path, "w") as file:
            listPrinter(file, (header if i == 0 else cpp))


def clearFiles():
    for path in glob.glob(targetpath.format("include", "Generated", "*"))\
                + glob.glob(targetpath.format("src", "Generated", "*")):
        os.unlink(path)


def generateSwitch(path: str):
    with open(path, "r") as analyzerFile:
        content = analyzerFile.readlines()

    # Drop first line with file marker
    content = content[1:]

    # Read metadata
    name = os.path.basename(path)
    classname = "GeneratedSwitch" + name[0].upper() + name[1:]

    # Read file into dict
    analyzers = []
    for line in content:
        # Strip \n if it exists and split line into fields
        fields = line.rstrip().split(" ")
        analyzers.append(Analyzer(fields[1], fields[0]))

    # Create header with analyzer objects
    header = [
        "#pragma once",
        '#include "analyzers/All.h"',
        '#include "dispatchers/metaprogramming/IMeta.h"',
        "class " + classname + " : public IMeta {",
        "public:",
        [
            "IAnalyzer* lookup(identifier_t identifier) override;"
        ],
        "private:"
    ]
    declarations = [
        "void stringifyAnalyzersState(std::ostream &os) const override;"
    ]
    for analyzer in analyzers:
        declarations.append(analyzer.getDeclaration())
    header.append(declarations)
    header.append("};")

    # Create lookup implementation cpp
    cpp = [
        '#include "dispatchers/metaprogramming/' + classname + '.h"',
        "IAnalyzer* " + classname + "::lookup(identifier_t identifier) {"
    ]
    functionContent = [
        "switch (identifier) {"
    ]
    for idx, analyzer in enumerate(analyzers):
        functionContent.append("case 0x" + analyzer.identifier + ":")
        functionContent.append([
            "return &" + analyzer.getCanonicalName() + ";",
        ])
    functionContent.append("default:")
    functionContent.append([
        "return nullptr;"
    ])
    functionContent.append("}")
    cpp.append(functionContent)
    cpp.append("}")
    cpp.append("void " + classname + "::stringifyAnalyzersState(std::ostream &os) const {")
    functionContent = []
    for idx, analyzer in enumerate(analyzers):
        functionContent.append("os << " + analyzer.getCanonicalName() + " << \"\\n\";")
    cpp.append(functionContent)
    cpp.append("}")

    writeFiles(classname, header, cpp)


def generateIf(path: str):
    with open(path, "r") as analyzerFile:
        content = analyzerFile.readlines()

    # Drop first line with file marker
    content = content[1:]

    # Read metadata
    name = os.path.basename(path)
    classname = "GeneratedIf" + name[0].upper() + name[1:]

    # Read file into dict
    analyzers = []
    for line in content:
        # Strip \n if it exists and split line into fields
        fields = line.rstrip().split(" ")
        analyzers.append(Analyzer(fields[1], fields[0]))

    # Create header with analyzer objects
    header = [
        "#pragma once",
        '#include "analyzers/All.h"',
        '#include "dispatchers/metaprogramming/IMeta.h"',
        "class " + classname + " : public IMeta {",
        "public:",
        [
            "IAnalyzer* lookup(identifier_t identifier) override;"
        ],
        "private:"
    ]
    declarations = [
        "void stringifyAnalyzersState(std::ostream &os) const override;"
    ]
    for analyzer in analyzers:
        declarations.append(analyzer.getDeclaration())
    header.append(declarations)
    header.append("};")

    # Create lookup implementation cpp
    cpp = [
        '#include "dispatchers/metaprogramming/' + classname + '.h"',
        "IAnalyzer* " + classname + "::lookup(identifier_t identifier) {"
    ]
    functionContent = []
    for idx, analyzer in enumerate(analyzers):
        functionContent.append(("" if idx == 0 else "else ") + "if (identifier == 0x" + analyzer.identifier + ") {")
        functionContent.append([
            "return &" + analyzer.getCanonicalName() + ";"
        ])
        functionContent.append("}")
    functionContent.append("else {")
    functionContent.append([
        "return nullptr;"
    ])
    functionContent.append("}")
    cpp.append(functionContent)
    cpp.append("}")
    cpp.append("void " + classname + "::stringifyAnalyzersState(std::ostream &os) const {")
    functionContent = []
    for idx, analyzer in enumerate(analyzers):
        functionContent.append("os << " + analyzer.getCanonicalName() + " << \"\\n\";")
    cpp.append(functionContent)
    cpp.append("}")

    writeFiles(classname, header, cpp)


def generateArray(path: str):
    with open(path, "r") as analyzerFile:
        content = analyzerFile.readlines()

    # Drop first line with file marker
    content = content[1:]

    # Read metadata
    name = os.path.basename(path)
    classname = "GeneratedArray" + name[0].upper() + name[1:]

    # Read file into dict
    analyzers = []
    for line in content:
        # Strip \n if it exists and split line into fields
        fields = line.rstrip().split(" ")
        analyzers.append(Analyzer(fields[1], fields[0]))
    analyzers.sort(key=lambda x: int(x.identifier, 16))

    # Create header with analyzer objects
    header = [
        "#pragma once",
        '#include "analyzers/All.h"',
        '#include "dispatchers/metaprogramming/IMeta.h"',
        "class " + classname + " : public IMeta {",
        "public:",
        [
            "IAnalyzer* lookup(identifier_t identifier) override;"
        ],
        "private:"
    ]
    declarations = [
        "void stringifyAnalyzersState(std::ostream &os) const override;"
    ]
    for analyzer in analyzers:
        declarations.append(analyzer.getDeclaration())
    lowest_identifier = int(analyzers[0].identifier, 16)
    highest_identifier = int(analyzers[-1].identifier, 16)
    declarations.append("IAnalyzer* table[" + str(highest_identifier - lowest_identifier + 1) + "] = {")
    entries = []
    current_index = 0
    current_analyzer = analyzers[current_index]
    for idx in range(highest_identifier + 1):
        if idx < lowest_identifier:
            continue

        if current_analyzer and int(current_analyzer.identifier, 16) == idx:
            entries.append("&" + current_analyzer.getCanonicalName() + ",")
            current_index += 1
            if current_index < len(analyzers):
                current_analyzer = analyzers[current_index]
            else:
                current_analyzer = None
        else:
            entries.append("nullptr,")
    declarations.append(entries)
    declarations.append("};")
    header.append(declarations)
    header.append("};")

    # Create lookup implementation cpp
    cpp = [
        '#include "dispatchers/metaprogramming/' + classname + '.h"',
        "IAnalyzer* " + classname + "::lookup(identifier_t identifier) {"
    ]
    callanalyzer_content = [
        "int64_t index = identifier - " + str(lowest_identifier) + ";",
        "if (index >= 0 && index < " + str(highest_identifier - lowest_identifier + 1) + " && table[index] != nullptr) {",
        [
            "return table[index];",
        ],
        "}",
        "return nullptr;",
    ]
    cpp.append(callanalyzer_content)
    cpp.append("}")
    cpp.append("void " + classname + "::stringifyAnalyzersState(std::ostream &os) const {")
    stringify_content = []
    for idx, analyzer in enumerate(analyzers):
        stringify_content.append("os << " + analyzer.getCanonicalName() + " << \"\\n\";")
    cpp.append(stringify_content)
    cpp.append("}")

    writeFiles(classname, header, cpp)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("type", choices=["if", "switch", "array", "all", "clean"])
    parser.add_argument("path", nargs="?", help="Input file with analyzer definitions.")
    args = parser.parse_args()

    if args.type == "all":
        analyzer_files = glob.glob(os.path.join(PROJECT_ROOT, "input", "analyzers", "*"))
        for analyzer_file in analyzer_files:
            # Check if path is valid
            if not os.path.isfile(analyzer_file):
                print("The given analyzer file is not a file.")
                exit(1)

            generateIf(analyzer_file)
            generateSwitch(analyzer_file)
            generateArray(analyzer_file)
        return

    if args.type == "clean":
        clearFiles()
        return

    # Check if path is given
    if not args.path:
        print("Path to analyzer file missing.")
        exit(1)

    # Check if path is valid
    if not os.path.isfile(args.path):
        print("The given analyzer file is not a file.")
        exit(1)

    if args.type == "if":
        generateIf(args.path)
    elif args.type == "switch":
        generateSwitch(args.path)
    elif args.type == "array":
        generateArray(args.path)


if __name__ == "__main__":
    main()
