const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});

    const mode = b.standardOptimizeOption(.{});
    const seqio = b.addStaticLibrary(.{
        .name = "seqio",
        .target = target,
        .optimize = mode,
    });

    seqio.linkLibC();
    seqio.addIncludePath(b.path("./deps/zlib"));

    seqio.addCSourceFiles(.{
        .root = b.path("./deps/zlib"),
        .files = &[_][]const u8{
            "adler32.c",
            "compress.c",
            "crc32.c",
            "deflate.c",
            "gzclose.c",
            "gzlib.c",
            "gzread.c",
            "gzwrite.c",
            "inflate.c",
            "infback.c",
            "inftrees.c",
            "inffast.c",
            "trees.c",
            "uncompr.c",
            "zutil.c",
        },
        .flags = &[_][]const u8{
            "-std=c89",
            "-O2",
        },
    });

    seqio.addCSourceFiles(.{
        .root = b.path("./"),
        .files = &[_][]const u8{
            "seqio.c",
        },
        .flags = &[_][]const u8{
            "-std=c99",
            "-O3",
        },
    });

    b.installArtifact(seqio);
}
