# Create virtual environment if it doesn't exist
python3 -m venv venv
# Activate it
source venv/bin/activate
# Install dependencies
pip install stem requests pysocks
# Install your package in development mode
pip install -e .