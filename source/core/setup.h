#ifndef VERDANT_SETUP_H
#define VERDANT_SETUP_H
/* First-launch installation from the self-contained Vita VPK. Runs before
 * guest RAM allocation/boot and never overwrites the user's Linux disk. */
static bool vs_install(uint64_t *tick) {
  if (strcmp(PLAT_SLUG, "vita"))
    return true;
  FILE *plan = fopen("app0:/setup-files/manifest.txt", "rb");
  if (!plan)
    return true; /* Thin packages used by the built-in update channel. */
  char version[48], line[768], installed[48] = {0};
  if (!fgets(version, sizeof(version), plan)) {
    fclose(plan);
    return false;
  }
  version[strcspn(version, "\r\n")] = 0;
  if (strcmp(version, VU_VERSION)) {
    fclose(plan);
    /* A thin auto-update leaves the old embedded setup bundle installed.
       The updater has already provided the new runtime in writable storage. */
    if (vu_exists(PLAT_SD "verdant/Image"))
      return true;
    term_printf("Install the latest standalone VPK to restore the runtime.\n");
    return false;
  }
  FILE *marker = fopen(PLAT_SD "verdant/setup-version.txt", "rb");
  if (marker) {
    if(!fgets(installed, sizeof(installed), marker))installed[0]=0;
    fclose(marker);
    installed[strcspn(installed, "\r\n")] = 0;
  }
  bool fresh = strcmp(installed, version) != 0;
  if (fresh) {
    term_printf(
        "Verdant first-launch setup\nInstalling included Linux runtime...\n");
    PresentTopScreen(tick);
  }
  bool image = false;
  int count = 0;
  while (fgets(line, sizeof(line), plan)) {
    line[strcspn(line, "\r\n")] = 0;
    if (strlen(line) < 66 || line[64] != ' ' || ++count > 32) {
      fclose(plan);
      return false;
    }
    line[64] = 0;
    const char *name = line + 65;
    if (strcmp(name, "verdant/Image") && strncmp(name, "verdant/guest/", 14)) {
      fclose(plan);
      return false;
    }
    if (!vu_name(name)) {
      fclose(plan);
      return false;
    }
    if (!strcmp(name, "verdant/Image"))
      image = true;
    char source[900], dest[900], part[932], backup[932];
    snprintf(source, sizeof(source), "app0:/setup-files/%s", name);
    snprintf(dest, sizeof(dest), PLAT_SD "%s", name);
    if (!fresh && vu_exists(dest))
      continue;
    if (vu_hash(dest, line))
      continue;
    snprintf(part, sizeof(part), "%s.setup-part", dest);
    snprintf(backup, sizeof(backup), PLAT_SD "verdant/setup-backup/%s/%s",
             version, name);
    if (!vu_parents(dest)) {
      term_printf("Cannot create runtime folders.\n");
      fclose(plan);
      return false;
    }
    FILE *in = fopen(source, "rb"), *out = fopen(part, "wb");
    if (!in || !out) {
      if (in)
        fclose(in);
      if (out)
        fclose(out);
      term_printf("Cannot open setup source/storage.\n");
      fclose(plan);
      return false;
    }
    unsigned char buffer[32768];
    size_t n;
    uint64_t bytes = 0, last = 0;
    bool ok = true;
    while ((n = fread(buffer, 1, sizeof(buffer), in))) {
      if (fwrite(buffer, 1, n, out) != n) {
        ok = false;
        break;
      }
      bytes += n;
      if (plat_us() - last > 1000000) {
        term_printf("Installing %s: %lu KiB\n", name,
                    (unsigned long)(bytes / 1024));
        PresentTopScreen(tick);
        last = plat_us();
      }
    }
    if (ferror(in) || ferror(out))
      ok = false;
    fclose(in);
    if (fclose(out))
      ok = false;
    if (!ok || !vu_hash(part, line)) {
      term_printf("Setup copy failed verification.\n");
      remove(part);
      fclose(plan);
      return false;
    }
    if (vu_exists(dest)) {
      if (!vu_parents(backup) || (!vu_exists(backup) && rename(dest, backup))) {
        term_printf("Could not preserve prior program file.\n");
        fclose(plan);
        return false;
      }
    }
    if (rename(part, dest)) {
      term_printf("Could not commit installed runtime file.\n");
      fclose(plan);
      return false;
    }
  }
  bool valid = image && !ferror(plan);
  fclose(plan);
  if (!valid)
    return false;
  if(!fresh)return true;
  marker = fopen(PLAT_SD "verdant/setup-version.part", "wb");
  if (!marker)
    return false;
  bool ok = fprintf(marker, "%s\n", version) > 0 && !ferror(marker);
  if (fclose(marker))
    ok = false;
  if(ok)remove(PLAT_SD "verdant/setup-version.txt");
  if (!ok || rename(PLAT_SD "verdant/setup-version.part",
                    PLAT_SD "verdant/setup-version.txt"))
    return false;
  if (fresh) {
    term_printf("Setup complete. Starting Linux...\n");
    PresentTopScreen(tick);
  }
  return true;
}
#endif
