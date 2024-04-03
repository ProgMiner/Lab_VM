package ru.byprogminer.lamagraalvm;

import org.graalvm.polyglot.Context;
import org.graalvm.polyglot.Source;

import java.io.File;
import java.io.IOException;

public final class Main {

    private static final String LAMA = "lama";

    public static void main(String[] args) {
        if (args.length != 1) {
            System.err.println("Usage: lama <filename>");
            System.exit(0);
        }

        final String path = args[0];
        final Source src;

        try {
            src = Source.newBuilder(LAMA, new File(path)).build();
        } catch (IOException e) {
            System.err.println("Unable to read file " + path);
            e.printStackTrace(System.err);
            System.exit(1);
            return;
        }

        try (final Context ctx = Context.create(LAMA)) {
            ctx.eval(src);
        }
    }
}
