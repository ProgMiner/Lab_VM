package ru.byprogminer.lamagraalvm;

import com.oracle.truffle.api.dsl.TypeSystem;
import ru.byprogminer.lamagraalvm.runtime.LamaArray;
import ru.byprogminer.lamagraalvm.runtime.LamaFun;
import ru.byprogminer.lamagraalvm.runtime.LamaRef;
import ru.byprogminer.lamagraalvm.runtime.LamaSexp;

@TypeSystem({long.class, LamaRef.class, String.class, LamaArray.class, LamaSexp.class, LamaFun.class})
public class LamaTypes {}
