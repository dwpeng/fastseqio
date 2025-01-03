import setuptools
import platform
import re
import os


def get_shared_lib_path():
    for root, dirs, files in os.walk("build"):
        for file in files:
            if re.search(r"\.(pyd|so|dylib)$", file):
                return os.path.join(root, file)
    raise FileNotFoundError("Shared library not found")


def copy_file(src, dst):
    with open(src, "rb") as f:
        with open(dst, "wb") as g:
            g.write(f.read())
    print(f"Copy {src} to {dst}")


if platform.system() == "Darwin" or platform.system() == "Windows":
    extension = []
    shared_lib_path = get_shared_lib_path()
    print("###########################")
    print(shared_lib_path)
    print("###########################")
    target_path = "./python/src/fastseqio/_fastseqio"
    suffix = os.path.splitext(shared_lib_path)[1]
    target_path += suffix
    copy_file(shared_lib_path, target_path)
    package_data = {"fastseqio": ["*.so", "*.pyd", "*.dylib"]}

elif platform.system() == "Linux":
    extension = [
        setuptools.Extension(
            "_fastseqio",
            sources=["./seqio.c", "./python/fastseqio.cc"],
            include_dirs=[".", "python/pybind11/include"],
            extra_link_args=["-lz"],
        )
    ]
    package_data = {}

setuptools.setup(
    name="fastseqio",
    version="0.0.4",
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
