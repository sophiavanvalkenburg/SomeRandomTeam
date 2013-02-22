extern int gameResult(int buf[]) {

  int i;

  // horizontal wins
  for (i = 0; i < 3; i++) {
    if (buf[i*3] == 0 && buf[i*3 + 1] == 0 && buf[i*3 + 2] == 0)
      return 0; // O wins
    else if (buf[i*3] == 1 && buf[i*3 + 1] == 1 && buf[i*3 + 2] == 1) 
      return 1; // X wins
  }

  // vertical wins
  for (i = 0; i < 3; i++) {
    if (buf[i] == 0 && buf[i + 3] == 0 && buf[i + 6] == 0)
      return 0; // O wins
    else if (buf[i] == 1 && buf[i + 3] == 1 && buf[i + 6] == 1)
      return 1; // X wins
  }

  
  // diagonal wins
  if ((buf[0] == 0 && buf[4] == 0 && buf[8] == 0) || (buf[2] == 0 && buf[4] == 0 && buf[6] == 0))
    return 0; // O wins
  
  if ((buf[0] == 1 && buf[4] == 1 && buf[8] == 1) || (buf[2] == 1 && buf[4] == 1 && buf[6] == 1))
    return 1; // X wins

  // drawn games
  for (i = 0; i < 9; i++) {
    if (buf[i] == -1)
      break;
    return 2; // Game Drawn
  }


return -1; // game still in progress

}

extern void displayBoard(char buf[]) {

  int i;

  for (i = 0; i < 8; i++) {
    if ((i+1) % 3 == 0) 
      printf("%c\n-----\n", buf[i]); 
    else 
      printf("%c|", buf[i]);
  }
  printf("%c\n-----\n", buf[i]);
      
}
 

}
