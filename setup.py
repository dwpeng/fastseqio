import setuptools
import platform
import re
import subprocess
import shutil

def get_shared_lib_path():
    shutil.rmtree("build", ignore_errors=True)
    p = subprocess.run(["xmake", "clean"], check=True)
    p = subprocess.Popen(
        ["xmake build -v"], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    out, err = p.communicate()
    if p.returncode != 0:
        raise RuntimeError("Failed to build the shared library")
    out = out.decode("utf-8") + err.decode("utf-8")
    shared_lib_path = re.findall(r"-o ([\w\/\.]+[\.a-z]+)", out)
    return shared_lib_path[-1]

# Copy the shared library to the package directory
if platform.system() == "Darwin" or platform.system() == "Windows":
    extension = []
    shared_lib_path = get_shared_lib_path()
    if platform.system() == "Darwin":
        subprocess.run(
            ["cp", shared_lib_path, "./python/src/fastseqio/_fastseqio.dylib"], check=True
        )
    elif platform.system() == "Windows":
        subprocess.run(
            ["copy", shared_lib_path, "./python/src/fastseqio/_fastseqio.pyd"],
            check=True,
        )
    package_data = ({"fastseqio": ["*.so", "*.pyd", "*.dylib"]},)
elif platform.system() == "Linux":
    extension = [
        setuptools.Extension(
            "_fastseqio",
            sources=["./seqio.c", "./python/fastseqio.cc"],
            include_dirs=[".", "python/pybind11/include"],
            extra_compile_args=["-lz"],
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
