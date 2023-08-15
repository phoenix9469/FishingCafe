//Parse RFID string
String rfidparse() {
  char alpha = ((rfidread.charAt(0)));
  char bravo = ((rfidread.charAt(1)));
  char charlie = ((rfidread.charAt(2)));
  char delta = ((rfidread.charAt(3)));
  char echo = ((rfidread.charAt(4)));
  char foxtrot = ((rfidread.charAt(5)));
  char golf = ((rfidread.charAt(6)));
  char hotel = ((rfidread.charAt(7)));
  char india = ((rfidread.charAt(8)));
  char juliett = ((rfidread.charAt(9)));
  String s1(alpha);
  String s2(bravo);
  String s3(charlie);
  String s4(delta);
  String s5(echo);
  String s6(foxtrot);
  String s7(golf);
  String s8(hotel);
  String s9(india);
  String s10(juliett);
  String rfid = s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9 + s10;
  return rfid;
}

String uartparse() {
  String uartread = uart.readString();
  char alpha = ((uartread.charAt(0)));
  char bravo = ((uartread.charAt(1)));
  char charlie = ((uartread.charAt(2)));
  char delta = ((uartread.charAt(3)));
  char echo = ((uartread.charAt(4)));
  String s1(alpha);
  String s2(bravo);
  String s3(charlie);
  String s4(delta);
  String s5(echo);
  String uart = s1 + s2 + s3 + s4 + s5;
  return uart;
}
