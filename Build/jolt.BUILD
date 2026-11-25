load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "jolt",
    srcs = glob(["Jolt/**/*.cpp"], exclude = ["Jolt/Renderer/**"]),
    hdrs = glob(["Jolt/**/*.h", "Jolt/**/*.inl"]),
    copts = select({
        "@platforms//os:windows": ["/std:c++17"],
        "//conditions:default": ["-std=c++17"],
    }),
    includes = ["."],
    visibility = ["//visibility:public"],
)
