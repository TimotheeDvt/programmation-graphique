#version 330 core

flat in int TexIndex;

void main() {
    if (TexIndex == 9 || TexIndex == 8 || TexIndex == 4) {
        discard; // discard Redstone, Torches and Glass
    }
}