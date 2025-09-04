# uvm - Unnarize Verse Manager

**uvm** is a simple, dependency-free command-line tool for managing libraries and dependencies for the **Unnarize scripting language**. It helps you initialize projects, fetch libraries from the official [unnarize](https://github.com/unnarize) organization, and manage your project's configuration file, `uvmpackage.json`.

It is written in C and compiled into a single, portable executable with no runtime dependencies.

-----

## Features

  * **Project Initialization**: Quickly scaffold a new Unnarize project.
  * **Dependency Management**: Easily add, install, and remove libraries.
  * **GitHub Integration**: Fetches modules directly from the official Unnarize language organization.
  * **Automatic Configuration**: Manages your `uvmpackage.json` and `.gitattributes` files automatically.
  * **Lightweight**: A single, small binary is all you need.

-----

## Installation

To install `uvm`, you will need `gcc` and `make` installed on your system.

**1. Clone the repository:**

```
git clone https://github.com/unnarize/uvm.git
cd uvm
```

**2. Compile the source code:**

```
make
```

**3. Install the binary:**
This command will copy the `uvm` executable to `/usr/local/bin`, making it available system-wide.

```
sudo make install
```

-----

## Usage

`uvm` provides a set of simple commands to manage your project's entire lifecycle.

### **`uvm init`**

Initializes a new Unnarize project in the current directory.

This command creates two files:

1.  **`uvmpackage.json`**: A configuration file to track your project's dependencies.

    ```
    {
      "name": "my-unnarize-project",
      "dependencies": []
    }
    ```

2.  **`.gitattributes`**: A file that tells GitHub's Linguist to correctly identify `.gi` files as the "Unnarize" language.

    ```
    # Tell GitHub's Linguist how to classify .gi files
    *.gi linguist-language=Unnarize
    ```

### **`uvm get <repo-name>`**

Fetches a single library from the `unnarize` GitHub organization, installs it, and adds it as a dependency to your `uvmpackage.json` file.

*Example: Adding the `lib-http` library.*

```
uvm get lib-http
```

This command will:

1.  Clone `https://github.com/unnarize/lib-http.git` into a new `umods/lib-http` directory.
2.  Remove the `.git` folder from the cloned library to keep your project clean.
3.  Automatically update your `uvmpackage.json` file.

*Before:*

```
{
  "name": "my-unnarize-project",
  "dependencies": []
}
```

*After:*

```
{
  "name": "my-unnarize-project",
  "dependencies": [
    "lib-http"
  ]
}
```

### **`uvm install`**

Reads the `uvmpackage.json` file and installs all the dependencies listed in the `dependencies` array. This command is perfect for setting up a project after you've cloned it from a repository.

```
uvm install
```

This will download and install every library specified in the config file into the `umods/` directory.

### **`uvm uninstall <repo-name>`**

Removes a library from your project.

*Example: Removing the `lib-http` library.*

```
uvm uninstall lib-http
```

This command will:

1.  Delete the `umods/lib-http` directory from your filesystem.
2.  Automatically update your `uvmpackage.json` file to remove the dependency.

*Before:*

```
{
  "name": "my-unnarize-project",
  "dependencies": [
    "lib-http",
    "lib-json"
  ]
}
```

*After:*

```
{
  "name": "my-unnarize-project",
  "dependencies": [
    "lib-json"
  ]
}
```

### **`uvm --version`**

Displays the current version of the `uvm` tool. You can also use the `-v` flag.

```
uvm --version
# or
uvm -v
```

-----

## Building from Source

If you prefer not to install the tool globally, you can compile it and run it from the project directory.

**Compile:**

```
make
```

**Run:**

```
./uvm <command>
```
---

## Author

**Gtkrshnaaa**
