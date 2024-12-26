import setuptools
import platform

is_windows = platform.system() == "Windows"


def find_zlib_path_on_windows() -> str:
    import os
    zlib_path = os.path.join("vcpkg", "installed", "x64-windows-release", "lib")
    return os.path.join("C:", zlib_path)


libraries = []
library_dirs = []
if is_windows:
    library_dirs.append(find_zlib_path_on_windows())
    libraries.append("zlib")
else:
    libraries.append("z")


extensions = [
    setuptools.Extension(
        name="_fastseqio",
        sources=["./python/fastseqio.cc", "seqio.c"],
        include_dirs=[".", "python/pybind11/include"],
        extra_compile_args=[],
        libraries=libraries,
        library_dirs=library_dirs,
    )
]

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
    ext_modules=extensions,
)
