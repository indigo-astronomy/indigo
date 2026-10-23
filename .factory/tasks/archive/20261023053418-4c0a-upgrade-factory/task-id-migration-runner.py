"""Trusted local migration runner: exclude INDIGO protected/vendor/legacy trees before inventory reads."""
import sys
sys.path.insert(0, '/Users/polakovic/Development/indigo/.factory/scripts')
import task_upgrade as migration
original_git = migration.git
protected = ('indigo_drivers/ccd_apogee/', 'indigo_drivers/ccd_sbig/', 'indigo_drivers/mount_asi/', 'indigo_drivers/mount_mxhd/', 'indigo_drivers/system_ascol/', 'indigo_tests/', 'obsoleted_drivers/', 'build/')
def scoped_git(root, *args):
    value = original_git(root, *args)
    if args and args[0] == 'ls-files':
        value = '\0'.join(name for name in value.split('\0') if (name.startswith(('.factory/', 'docs/decisions/')) or name.endswith(('.md', '.mdc'))) and not name.startswith(protected) and not {'externals', 'bin_externals'}.intersection(name.split('/')))
    return value
migration.git = scoped_git
sys.exit(migration.main())
