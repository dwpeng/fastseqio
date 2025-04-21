import setuptools

import sys

if sys.platform == "linux":
    extension = [
        setuptools.Extension(
            "_fastseqio",
            sources=["./seqio.c", "./python/fastseqio.cc"],
            include_dirs=[".", "python/pybind11/include"],
            extra_link_args=["-lz"],
        )
    ]
    package_data = {}
elif sys.platform == "win32":
    extension = [
        setuptools.Extension(
            "_fastseqio",
            sources=["./seqio.c", "./python/fastseqio.cc"],
            include_dirs=[".", "python/pybind11/include", "./deps/zlib"],
            extra_objects=["./zig-out/lib/z.lib"],
        )
    ]
    package_data = {}
else:
    raise ValueError("")

setuptools.setup(
    name="fastseqio",
    version="0.0.9",
    author="dwpeng",
    author_email="1732889554@qq.com",
    license="MIT",
    url="https://github.com/dwpeng/fastseqio",
    description="A package for reading and writing fasta/fastq files",
    packages=setuptools.find_namespace_packages(where="./python/src"),
    package_dir={"": "./python/src"},
    ext_modules=extension,
    package_data=package_data,  # type: ignore
)
