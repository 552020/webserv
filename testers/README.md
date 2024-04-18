# Testers

- https://github.com/hygoni/webserv_tester

## Hygoni

How to set up a virtual environemnt to avoid problems deriving from multiple Python instalations. Using a virtual environment allows to manage dependencies separately from the global Python installation. This practice prevents conflicts between project requirements and system-wide packages, especially in environments with multiple Python versions.

## Pre-requisites

- Python 3.x installed on your system
- Access to terminal on macOS or Linux

## Steps to Set Up the Environment

### Step 1: Identify Python Installation

Ensure you are using the correct Python version for the project. You can find your Python installation paths and select the appropriate version if multiple instances are installed:

```bash
which -a python3
```

Select the desired Python executable for the next steps.

### Step 2: Create a Virtual Environment

Create a virtual environment to isolate your project dependencies. Replace `/path/to/python3` with the path to the desired Python executable from Step 1 and `path/to/your/project` with your project directory.

```bash
/path/to/python3 -m venv /path/to/your/project/venv
```

### Step 3: Activate the Virtual Environment

Activate the virtual environment. You must do this every time you start working on the project in a new terminal session.

```bash
source /path/to/your/project/venv/bin/activate
```

### Step 4: Install Dependencies

With the virtual environment activated, install your project’s dependencies:

```bash
pip install -r requirements.txt
```

Ensure to maintain a `requirements.txt` file at the root of your project directory detailing all necessary packages.

### Step 5: Verify Installation

Check the installation of dependencies within the virtual environment:

```bash
pip list
```

### Step 6: Run Your Project

Execute your project’s main script or application:

```bash
python path/to/your/script.py
```

## Additional Tips

- Always ensure the virtual environment is activated while working on the project to avoid using system-wide Python packages.
- To exit the virtual environment, you can use the `deactivate` command.

## Troubleshooting

- **ModuleNotFoundError**: Ensure the virtual environment is activated and the required package is installed within it.
- **Permission Issues on macOS/Linux**: Use `sudo` with caution when dealing with Python installations managed by the system or third-party package managers like Homebrew or Apt.

For platform-specific issues or further assistance, consult your team's developer documentation or system administrator.

```

This revised README is streamlined for macOS and Ubuntu users and should be suitable for sharing among developers who are working on these operating systems. If you need any more modifications or additional content, feel free to let me know!
```
