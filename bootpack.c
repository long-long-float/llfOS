void io_hlt();

void HariMain() {
fin:
  io_hlt();
  goto fin;
}
