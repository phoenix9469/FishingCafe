//Read button, sound buzzer, control LEDs
void btnread() {

  if (mcp.digitalRead(netbtn) == LOW && netprv == HIGH && millis() - time > debounce) {
    if (netst == HIGH) {
      netst = LOW;
    } else {
      netst = HIGH;
    }
    time = millis();
    tone(buzzer, 2680);
    delay(200);
  } else {
    noTone(buzzer);
  }

  if (mcp.digitalRead(tarebtn) == LOW) {
    Serial.println("tare");
    uart.listen();
    uart.print("TR");
    tone(buzzer, 2680);
    mcp.digitalWrite(tareled, HIGH);
    delay(200);
    mcp.digitalWrite(tareled, LOW);
  } else {
    noTone(buzzer);
  }

  if (mcp.digitalRead(btn1) == LOW && btn1prv == HIGH && millis() - time > debounce) {
    if (btn1st == HIGH) {
      btn1st = LOW;
    } else {
      btn1st = HIGH;
      btn2st = LOW;
      btn3st = LOW;
      btn4st = LOW;
    }
    time = millis();
    tone(buzzer, 2680);
    delay(200);
  } else {
    noTone(buzzer);
  }

  if (mcp.digitalRead(btn2) == LOW && netprv == HIGH && millis() - time > debounce) {
    if (btn2st == HIGH) {
      btn2st = LOW;
    } else {
      btn2st = HIGH;
      btn1st = LOW;
      btn3st = LOW;
      btn4st = LOW;
    }
    time = millis();
    tone(buzzer, 2680);
    delay(200);
  } else {
    noTone(buzzer);
  }

  if (mcp.digitalRead(btn3) == LOW && netprv == HIGH && millis() - time > debounce) {
    if (btn3st == HIGH) {
      btn3st = LOW;
    } else {
      btn3st = HIGH;
      btn1st = LOW;
      btn2st = LOW;
      btn4st = LOW;
    }
    time = millis();
    tone(buzzer, 2680);
    delay(200);
  } else {
    noTone(buzzer);
  }

  if (mcp.digitalRead(btn4) == LOW && netprv == HIGH && millis() - time > debounce) {
    if (btn4st == HIGH) {
      btn4st = LOW;
    } else {
      btn4st = HIGH;
      btn1st = LOW;
      btn2st = LOW;
      btn3st = LOW;
    }
    time = millis();
    tone(buzzer, 2680);
    delay(200);
  } else {
    noTone(buzzer);
  }

  mcp.digitalWrite(netled, netst);
  mcp.digitalWrite(btn1led, btn1st);
  mcp.digitalWrite(btn2led, btn2st);
  mcp.digitalWrite(btn3led, btn3st);
  mcp.digitalWrite(btn4led, btn4st);
}
