# Test the modules

You can test the command module easily using docker-compose. However
you need to make sure that the module is built for debian-like distros
since the mesos image is based on ubuntu image.

To test the module:
1. Build the module (see README at root of the repo).
2. Copy the module `libmesos_command_modules.so` in the `modules/` directory.
3. Run `docker-compose up -d`

Then, your cluster should spawn and you should be able to access
Marathon on port 8080.

The registered commands are listed in `modules/modules.json`.

## Modify the commands

If you want to test a custom version of the commands, edit the
module.json or the content of the bash scripts already registered and do
a `docker-compose down` and `docker-compose up -d` again to be sure
everything from the previous cluster has been cleaned up.
