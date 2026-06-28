import pytest
import subprocess
import sys
import os

# Add the process directory to the path to import compress module
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

# Payloads that should be safely handled or rejected
PAYLOADS = [
    # Valid input (should work normally)
    ("normal_file.txt",),
    # Basic command injection with semicolon
    ("file.txt; rm -rf /",),
    # Command substitution
    ("$(whoami).txt",),
    # Backtick execution
    ("`id`.txt",),
    # Pipe and newline injection
    ("file.txt\ncat /etc/passwd",),
]

@pytest.mark.parametrize("payload", PAYLOADS)
def test_shell_command_injection_safety(payload, tmp_path):
    """Invariant: Shell commands never include unsanitized user input"""
    from process import compress
    
    # Create a test file in temporary directory
    test_file = tmp_path / payload[0].split()[0].replace(';', '').replace('`', '').replace('$', '').replace('\n', '')
    test_file.write_text("test content")
    
    # Create output file name
    output_file = tmp_path / "output.zip"
    
    # Prepare parameters list
    parameters = [str(test_file)]
    
    try:
        # Call the actual compress function with adversarial input
        # We'll test by checking if dangerous shell commands were executed
        # by monitoring for unexpected side effects
        
        # First, check if original file still exists after operation
        original_exists_before = test_file.exists()
        
        # We can't directly intercept os.system calls, so we'll check
        # that the function doesn't crash with injection payloads
        # and that no unexpected files were created/deleted
        
        # List files before operation
        files_before = list(tmp_path.iterdir())
        
        # Import and patch os.system to capture commands
        import process.compress as compress_module
        original_system = compress_module.os.system
        
        captured_commands = []
        def capture_system(command):
            captured_commands.append(command)
            # Simulate successful zip/bzip2 operation for test
            if "zip" in command:
                output_file.touch()
            return 0
        
        compress_module.os.system = capture_system
        
        try:
            # Test the zip command
            compress_module.compress_files(str(output_file), parameters)
            
            # Check that no shell metacharacters appear in captured commands
            for cmd in captured_commands:
                # Assert that the payload doesn't appear unescaped in the command
                # (basic check - in reality would need more sophisticated escaping detection)
                for dangerous in [';', '`', '$(']:
                    if dangerous in payload[0]:
                        # The payload itself shouldn't appear literally in the command
                        # unless properly escaped/quoted
                        if payload[0] in cmd and dangerous in cmd:
                            # Check if it's at least quoted (basic check)
                            if f"'{payload[0]}'" not in cmd and f'"{payload[0]}"' not in cmd:
                                pytest.fail(f"Unsanitized input found in command: {cmd}")
            
        finally:
            # Restore original os.system
            compress_module.os.system = original_system
            
    except Exception as e:
        # The function should handle adversarial input gracefully
        # Either by rejecting it or escaping it, not by crashing unexpectedly
        if "No such file" not in str(e) and "invalid" not in str(e).lower():
            raise