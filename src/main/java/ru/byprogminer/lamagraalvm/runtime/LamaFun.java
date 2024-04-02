package ru.byprogminer.lamagraalvm.runtime;

import com.oracle.truffle.api.RootCallTarget;
import com.oracle.truffle.api.frame.MaterializedFrame;

public record LamaFun(RootCallTarget callTarget, MaterializedFrame parentScope) {}
