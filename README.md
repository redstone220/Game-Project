\# Flappy Bird

Once upon a time a bird loved to flap. So we made a game about it...



\## How to run :

Before running the code, make sure all required software are installed and added to your \*\*path\*\*.

\- VS code

\- GCC compiler

\- C standard \_\*\*C 11\*\*\_



Launch VS code (or any code editor) and open the folder containing all the code, assets and music. Open VS code terminal (press `ctrl + J`) and type the following command

```

gcc main.c -o main.exe -Iraylib/include -Lraylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm

./main.exe

```

A game window will open.



\## How to play:

Flappy bird can be played using  \*\*keyboard\*\* or  \*\*mouse\*\*. In title screen use mouse to navigate around menu. Press \*\*\_Space Button\_\*\* or \*\*\_Left Mouse Button\_\*\* to start the game. Control the bird by pressing the same button. You die if you hit any pipe or hit the ceiling or floor. Game speeds up as you pass more and more pipe.



\## Features :

Other than the main gameplay, this game contain the following features:

\- \*\*Customization\*\* : Birds, backgrounds, pipes can be customized from a wide variety of choices. Press the \_pencil\_ icon on the right side to customize the look. Press \_exit\_ icon or \_Enter Key\_ to exit customization.

\- \*\*Sound\*\* : Sound and music can be toggled on and off using the \_sound\_ icon on the right.

\- \*\*Player Name\*\* : Player name can be changed by pressing the \_player with pencil\_ icon on the right side of menu.

\- \*\*Hall of Fame\*\* :  Top five players are shown in the Hall of Fame leaderboard.

\- \*\*Credits and How to Play\*\* : Credits and playing instructions are shown here.





