import cray

if __name__ == "__main__":
	print("libc-ray version {} ({})".format(cray.version.semantic(), cray.version.hash()))
	with cray.renderer() as r:
		print("Initial value {}".format(r.prefs.threads))
		r.prefs.threads = 42
		print("Got value {}".format(r.prefs.threads))
		r.prefs.threads = 69
		print("Got value {}".format(r.prefs.threads))
		r.prefs.threads = 1234
		print("Got value {}".format(r.prefs.threads))
		r.prefs.asdf = 1234
		print("Default filename: {}".format(r.prefs.output_name))
