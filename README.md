 # What is this?

 A Qt Google Drive client with end-to-end/client-side encryption.



# TODO: 
- [ ] Split into multiple files (Refactor)
- [x] Show heirarchy of folder(s) instead of just listing all files
    - Probably: ```?orderBy=folder``` from https://developers.google.com/workspace/drive/api/reference/rest/v3/files/list#:~:text=%3ForderBy%3Dfolder
    - probably horrible idea since it's meant to be encrypted
- [ ] Figure out a sensible IV
- [x] QoL settings like hash/randomize/encrypt filenames (ew)
- [x] Catppucin theme
- [x] Add Ui lock to prevent user from deleting same entry twice.
    - It already returns an error message if they try to delete same file twice so it's not destructive
- [ ] Try and integrate with the local Gdrive client thing (this probably only exists on Windows)

- [x] Save configuration somewhere
- [x] CI with gh actions
- [x] Move away from using libraries for encryption (slow)

