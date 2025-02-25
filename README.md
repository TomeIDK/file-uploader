✅ Handling More File Types – Use magic numbers to validate file formats instead of relying on extensions.  
✅ Custom Upload Directories – Allow users to specify where in /uploads a file should be stored.  
✅ Threading Optimization – Improve multi-client handling beyond 5 clients, possibly using pthreads.  
✅ File Size Limits – Set a max file size to prevent overloads and crashes.  
✅ Logging – Keep track of uploaded files, errors, and client interactions.  
✅ Better Error Handling – Handle missing files, permission issues, and network failures gracefully.  
✅ Checksum Verification – Implement SHA-256 or MD5 to verify file integrity after transfer.  
✅ Authentication – Implement basic authentication to control access (e.g., passwords, API tokens).  
✅ Configurable Settings – Store server settings (e.g., max file size, allowed formats) in a config file (config.json or .ini).  
✅ Compression – Use Gzip or zlib to compress files before storage to save space.  
✅ Encryption – Encrypt files using AES-256 before storing them for added security.  
