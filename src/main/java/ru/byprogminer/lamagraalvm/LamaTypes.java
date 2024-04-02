package ru.byprogminer.lamagraalvm;

import com.oracle.truffle.api.dsl.TypeSystem;
import ru.byprogminer.lamagraalvm.runtime.*;

@TypeSystem({long.class, LamaRef.class, LamaString.class, LamaArray.class, LamaSexp.class, LamaFun.class})
public class LamaTypes {}
