package ru.byprogminer.lamagraalvm;

import lombok.Getter;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;

@Getter
public class LamaContext {

    private PrintWriter out;
    private BufferedReader in;

    public void init() {
        out = new PrintWriter(System.out);
        in = new BufferedReader(new InputStreamReader(System.in));
    }
}
