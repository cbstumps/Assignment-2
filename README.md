# Assignment-2
Part 1: Setting up this code is easy, all you need to do is upload the Reaction_time_game_code.ino onto your M5Stick and it will run automatically whether you keep the device plugged in or not.
  - Extra Feature 1 : I have implemented a live timer to show you how much time is passing by as the game is going
  - Extra Feature 2 : A leaderboard was added to track the top 5 best reaction times and store them on the flash memory, so even if the     device is turned off it will still keep the top times
Video Link : https://www.youtube.com/watch?v=Tocq8lvbmAU

Part 2: To set up this code: 
- First make sure the image folder you are pulling from is in the same folder as the Receive and Send images code, and make sure the folder name matches the name on the "send_images" code (the IMG_FOLDER variable). 
- Next make sure the PORT is the same on the python code and the Arduino IDE, and once you have some images in your folder you are ready to upload. 
- Navigate the terminal to the folder where you keep all the code and images and type in "py send_images.py"
- The code will run and the images will appear on the M5Stick!
  - Extra Feature 1 : There are multiple transitions the user can pick between in the C++ code, just type in one of the available numbers and upload the code to the M5 and the next slideshow will have different transitions
  - Extra Feature 2 : There is a progress bar that shows how much of the image has been uploaded to the M5 before it displays the transition into the next photo
  - Extra Feature 2.5 : Was not sure if this counted as a full additional feature since the button press already toggles the image, but I added a feature where holding down the M5 button will let the computer know that the M5Stick is ready to receive the next photo
Video Link : https://www.youtube.com/watch?v=5aQT68LpMi8
