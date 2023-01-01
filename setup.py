from setuptools import setup, Extension

my_module = Extension("lrd", 
		sources = ['python_bind.c'],
		extra_objects = ["./lib_lrdshared.a"]
		)

def main():
	setup(name="lrd",
		version="1.0.0",
		description="Python interface for the p2plink library function",
		author="<Georgios Kapetanos>",
		author_email="kapgeorgios@uth.gr",
		ext_modules=[my_module])

if __name__ == "__main__":
	main()
