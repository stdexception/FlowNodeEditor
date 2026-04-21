
# ![Logo](apps/execution_node_editor/assets/icons/64x64.png) NodeEditor

A multi purpose node editor for flow-based programming (C++/Qt).

## Description

The purpose of this node editor is to connect functional building blocks and parameterize them in order to form a graph which can then be executed using the execution subsystem.

### Execution Subsystem

For running the graph created with the node editor, some sort of flow-based programming library is needed.

If you want, you could swap the execution subsystem shipped in the Releases by your own system. It just needs to understand the node editor's output format of the graph.

### Node Type Definition

You can add your own node types by adding a `JSON` file to a subfolder of
`apps/execution_node_editor/execution_subsystem/node_type_definitions` containing your node attributes.

Here is one example:

```json
{
    "node_type": "GaussianBlurNode",
    "input_ports": [
      {
        "port_name": "image",
        "data_type": "image_t"
      }
    ],
    "output_ports": [
      {
        "port_name": "blurred",
        "data_type": "image_t"
      }
    ],
    "default_settings":
    {
      "sigma": 1.0
    }
}
```

### Output Formats

The node editor creates two files:

* The `Scene` with the file extension `.nes` saves the entire scene containing nodes, edges, node settings, viewport settings and user interface settings. It is designed to be loaded again by the node editor.
* The `Graph` with the file extension `.graph.json` saves only the nodes, edges and node settings. It is designed to be an exchange format for the execution subsystem and running the graph.

## License

This software is licensed under [MIT License](https://opensource.org/licenses/MIT).

## How To Build

### Prerequisites

- [CMake](https://cmake.org/download/) 3.16 or newer
- Qt 5.15+ or Qt 6.x with **Widgets** module installed
- A C++17 compiler (MSVC, GCC, or Clang)

Set `CMAKE_PREFIX_PATH` to your Qt installation if CMake cannot find Qt (for example `C:/Qt/6.5.0/msvc2019_64` on Windows).

### Configure and build

```sh
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build build --config Release
```

On Windows you can run `pack.bat` from the repository root; it configures `build/` if missing and builds the **Release** configuration.

The `FlowNodeEditor` executable is written to `build/Release/` (single-configuration generators such as Ninja use `build/` instead).

CMake copies `apps/execution_node_editor/assets` and `apps/execution_node_editor/execution_subsystem/node_type_definitions` next to the executable after each build so the editor can load styles, icons, fonts, and node JSON definitions at runtime.

## Credits

- This node editor is based on `pyqt-node-editor` by [Pavel Křupala](https://gitlab.com/pavel.krupala). Visit the original [repository](https://gitlab.com/pavel.krupala/pyqt-node-editor) on GitLab.

## Contribution

If you would like to contribute please read this [Contribution Guide](CONTRIBUTING.md)

## Reachout

Feel free to contact me if you have any questions: sebastian.beyer@live.com

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/beyse)
