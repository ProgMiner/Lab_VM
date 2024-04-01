package ru.byprogminer.lamagraalvm;

import org.graalvm.polyglot.Context;

public class Main {

    public static void main(String[] args) {
        try (final Context ctx = Context.create(LamaLanguage.ID)) {
            System.out.println(ctx.eval(LamaLanguage.ID, "100500"));
        }
    }
}
