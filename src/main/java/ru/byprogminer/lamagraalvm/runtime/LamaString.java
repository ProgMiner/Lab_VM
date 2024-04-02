package ru.byprogminer.lamagraalvm.runtime;

import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;

@SuppressWarnings("MethodDoesntCallSuperMethod")
public record LamaString(byte[] value) implements Cloneable {

    public static final Charset CHARSET = StandardCharsets.UTF_8;

    @Override
    public Object clone() {
        return new LamaString(value.clone());
    }

    @Override
    public String toString() {
        return new String(value, CHARSET);
    }

    public static LamaString ofString(String str) {
        return new LamaString(str.getBytes(CHARSET));
    }
}
