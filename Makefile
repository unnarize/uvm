# Compiler and compilation flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# File and directory variables
SRC = uvm.c
EXECUTABLE = uvm
INSTALL_DIR = /usr/local/bin

# --- Targets ---

.PHONY: all clean install uninstall

# Default target: build the executable
all: $(EXECUTABLE)

# Rule to build the executable from the source file
$(EXECUTABLE): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

# Remove the compiled executable
clean:
	rm -f $(EXECUTABLE)

# Install the executable to the system path (requires sudo)
install: all
	@echo "Installing $(EXECUTABLE) to $(INSTALL_DIR)..."
	@# Create the installation directory if it doesn't exist
	sudo install -d -m 0755 $(INSTALL_DIR)
	@# Install the executable with proper permissions (read/write/execute for owner, read/execute for others)
	sudo install -m 0755 $(EXECUTABLE) $(INSTALL_DIR)
	@echo "Successfully installed. You can now run 'uvm' from anywhere."

# Uninstall the executable from the system path (requires sudo)
uninstall:
	@echo "Uninstalling $(EXECUTABLE) from $(INSTALL_DIR)..."
	sudo rm -f $(INSTALL_DIR)/$(EXECUTABLE)
	@echo "Successfully uninstalled."