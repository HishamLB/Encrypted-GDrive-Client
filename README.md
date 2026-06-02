 # What is this?

 A Qt Google Drive client with end-to-end/client-side encryption.



# TODO: 
- [ ] Split into multiple files (Refactor)
- [ ] Show heirarchy of folder(s) instead of just listing all files
    - Probably: ```?orderBy=folder``` from https://developers.google.com/workspace/drive/api/reference/rest/v3/files/list#:~:text=%3ForderBy%3Dfolder
    - probably horrible idea since it's meant to be encrypted
- [ ] Figure out a sensible IV
- [ ] QoL settings like hash/randomize/encrypt filenames (ew)
- [x] Catppucin theme
- [ ] Add Ui lock to prevent user from deleting same entry twice.
    - It already returns an error message if they try to delete same file twice so it's not destructive
