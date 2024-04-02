package ru.byprogminer.lamagraalvm.runtime;

@SuppressWarnings("MethodDoesntCallSuperMethod")
public record LamaString(byte[] value) implements Cloneable {

    @Override
    public Object clone() {
        return new LamaString(value.clone());
    }
}
