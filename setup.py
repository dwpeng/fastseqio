import setuptools

import sys

if sys.platform == "linux":
    extension = [
        setuptools.Extension(
            "_fastseqio",
            sources=["./python/fastseqio.cc"],
            include_dirs=[".", "python/pybind11/include", "./deps/zlib"],
            extra_objects=["./zig-out/lib/libseqio.a"],
            extra_compile_args=["-std=c++11"],
        )
    ]
elif sys.platform == "win32":
    extension = [
        setuptools.Extension(
            "_fastseqio",
            sources=["./python/fastseqio.cc"],
            include_dirs=[".", "python/pybind11/include", "./deps/zlib"],
            extra_objects=["./zig-out/lib/seqio.lib"],
        )
    ]
elif sys.platform == "darwin":
    extension = [
        setuptools.Extension(
            "_fastseqio",
            sources=["./python/fastseqio.cc"],
            include_dirs=[".", "python/pybind11/include", "./deps/zlib"],
            extra_objects=["./zig-out/lib/libseqio.a"],
            extra_compile_args=["-std=c++11"],
        )
    ]
else:
    raise ValueError("Unsupported platform: " + sys.platform)

setuptools.setup(
    name="fastseqio",
    version="0.1.7",
    author="dwpeng",
    author_email="1732889554@qq.com",
    license="MIT",
    url="https://github.com/dwpeng/fastseqio",
    description="A package for reading and writing fasta/fastq files",
    packages=setuptools.find_namespace_packages(where="./python/src"),
    package_dir={"": "./python/src"},
    ext_modules=extension,
)
