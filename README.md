# Flappy Bird
Once upon a time a bird loved to flap. So we made a game about it...

## How to run:
Before running the code, make sure all required software are installed and added to your **path**.
- VS Code
- GCC compiler
- C standard **C11**

Launch VS Code (or any code editor) and open the folder containing all the code, assets, and music. Open the VS Code terminal (press `Ctrl + J`) and type the following command:

```
gcc main.c -o main.exe -Iraylib/include -Lraylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm
./main.exe
```

A game window will open.

## How to play:
Flappy bird can be played using **keyboard** or **mouse**. In the title screen, use the mouse to navigate around the menu. Press **Space** or the **Left Mouse Button** to start the game. Control the bird by pressing the same button. You die if you hit any pipe, the ceiling, or the floor. The game speeds up as you pass more pipes.

## Features:
Other than the main gameplay, this game contains the following features:
- **Customization:** Birds, backgrounds, and pipes can be customized from a wide variety of choices. Press the *pencil* icon on the right side to customize the look. Press the *exit* icon or the **Enter** key to exit customization.
- **Sound:** Sound and music can be toggled on and off using the *sound* icon on the right.
- **Player Name:** Player name can be changed by pressing the *player with pencil* icon on the right side of the menu.
- **Hall of Fame:** Top five players are shown in the Hall of Fame leaderboard.
- **Credits and How to Play:** Credits and playing instructions are shown here.
