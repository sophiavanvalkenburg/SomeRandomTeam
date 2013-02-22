void displayBoard(char buf[]) {

  int i;

  for (i = 0; i < 8; i++) {
    if ((i+1) % 3 == 0) 
      printf("%c\n-----\n", buf[i]); 
    else 
      printf("%c|", buf[i]);
  }
  printf("%c\n-----\n", buf[i]);
      
}

int gameResult(char buf[]) {

  int i, j;

  // horizontal wins
  for (i = 0; i < 3; i++) {
    if (buf[i*3] == 'X' && buf[i*3 + 1] == 'X' && buf[i*3 + 2] == 'X')
      return 0; // X wins
    else if (buf[i*3] == 'O' && buf[i*3 + 1] == 'O' && buf[i*3 + 2] == 'O') 
      return 1; // O wins
  }

  // vertical wins
  for (i = 0; i < 3; i++) {
    if (buf[i] == 'X' && buf[i + 3] == 'X' && buf[i + 6] == 'X')
      return 0; // X wins
    else if (buf[i] == 'O' && buf[i + 3] == 'O' && buf[i + 6] == 'O')
      return 1; // O wins
  }

  
  // diagonal wins
  if ((buf[0] == 'X' && buf[4] == 'X' && buf[8] == 'X') || (buf[2] == 'X' && buf[4] == 'X' && buf[6] == 'X'))
    return 0; // X wins
  
  if ((buf[0] == 'O' && buf[4] == 'O' && buf[8] == 'O') || (buf[2] == 'O' && buf[4] == 'O' && buf[6] == 'O'))
    return 1; // O wins

  // drawn games
  for (i = 0; i < 9; i++) {
    if ((buf[i] != 'X') && (buf[i] != 'O'))
      break;
    return 2; // Game Drawn
  }


return -1; // game still in progress

}
