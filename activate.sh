#!/bin/bash

# Define colors for terminal output
GREEN='\033[0;32m'  # Green for success messages
BLUE='\033[0;34m'   # Blue for informational messages
CYAN='\033[0;36m'   # Cyan for titles and headings
NC='\033[0m'        # No Color - resets text to default terminal color

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Show a welcome message
echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Toralizer Development Environment${NC}"
echo -e "${CYAN}========================================${NC}"

# Show activation message
echo -e "\n${BLUE}Activating environment...${NC}"

# Activate the virtual environment
source "${SCRIPT_DIR}/venv/bin/activate"

# Confirm activation with a success message
echo -e "\n${GREEN}✓ Environment activated successfully!${NC}"
echo -e "${GREEN}You're now ready to work on Toralizer.${NC}"
echo -e "\n${BLUE}Remember: Type '${CYAN}deactivate${BLUE}' when you're done.${NC}"