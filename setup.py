import setuptools

import sys

if sys.platform == "linux":
    extension = [
        setuptools.Extension(
            "_fastseqio",
            sources=["./seqio.c", "./python/fastseqio.cc"],
            include_dirs=[".", "python/pybind11/include", "./deps/zlib"],
            extra_objects=["./zig-out/lib/libz.a"],
        )
    ]
elif sys.platform == "win32":
    extension = [
        setuptools.Extension(
            "_fastseqio",
            sources=["./seqio.c", "./python/fastseqio.cc"],
            include_dirs=[".", "python/pybind11/include", "./deps/zlib"],
            extra_objects=["./zig-out/lib/z.lib"],
        )
    ]
elif sys.platform == "darwin":
    extension = [
        setuptools.Extension(
            "_fastseqio",
            sources=["./seqio.c", "./python/fastseqio.cc"],
            include_dirs=[".", "python/pybind11/include", "./deps/zlib"],
            extra_objects=["./zig-out/lib/libz.a"],
            extra_compile_args=["-Wc++11-extensions"],
        )
    ]
else:
    raise ValueError("Unsupported platform: " + sys.platform)

setuptools.setup(
    name="fastseqio",
    version="0.1.2",
    author="dwpeng",
    author_email="1732889554@qq.com",
    license="MIT",
    url="https://github.com/dwpeng/fastseqio",
    description="A package for reading and writing fasta/fastq files",
    packages=setuptools.find_namespace_packages(where="./python/src"),
    package_dir={"": "./python/src"},
    ext_modules=extension,
)
