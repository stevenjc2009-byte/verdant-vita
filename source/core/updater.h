#ifndef VERDANT_UPDATER_H
#define VERDANT_UPDATER_H
#include <errno.h>
#include <sys/stat.h>
#define VU_VERSION "0.2.2"
#ifdef __linux__
#include <openssl/evp.h>
#else
#include <mbedtls/sha256.h>
#include <mbedtls/version.h>
#endif
static bool vu_hash(const char *path, const char *expected) {
  unsigned char digest[32], buf[32768];
  size_t n;
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
#ifdef __linux__
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx) {
    fclose(f);
    return false;
  }
  int ok = EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
  while ((n = fread(buf, 1, sizeof(buf), f)))
    if (!EVP_DigestUpdate(ctx, buf, n))
      ok = 0;
  if (!EVP_DigestFinal_ex(ctx, digest, NULL))
    ok = 0;
  EVP_MD_CTX_free(ctx);
  if (!ok) {
    fclose(f);
    return false;
  }
#else
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
#if MBEDTLS_VERSION_MAJOR >= 3
  int ok = mbedtls_sha256_starts(&ctx, 0);
  while ((n = fread(buf, 1, sizeof(buf), f)))
    if (mbedtls_sha256_update(&ctx, buf, n))
      ok = -1;
  if (mbedtls_sha256_finish(&ctx, digest))
    ok = -1;
#else
  int ok = mbedtls_sha256_starts_ret(&ctx, 0);
  while ((n = fread(buf, 1, sizeof(buf), f)))
    if (mbedtls_sha256_update_ret(&ctx, buf, n))
      ok = -1;
  if (mbedtls_sha256_finish_ret(&ctx, digest))
    ok = -1;
#endif
  mbedtls_sha256_free(&ctx);
  if (ok) {
    fclose(f);
    return false;
  }
#endif
  bool good = !ferror(f);
  fclose(f);
  char hex[65];
  for (int i = 0; i < 32; i++)
    sprintf(hex + i * 2, "%02x", digest[i]);
  return good && !strcmp(hex, expected);
}
static bool vu_exists(const char *p) {
  struct stat s;
  return !stat(p, &s);
}
static bool vu_parents(const char *p) {
  char b[768];
  if (strlen(p) >= sizeof(b))
    return false;
  strcpy(b, p);
  for (char *s = b; *s; s++)
    if (*s == '/' && s != b && s[-1] != ':') {
      *s = 0;
      if (mkdir(b, 0777) && errno != EEXIST)
        return false;
      *s = '/';
    }
  return true;
}
static bool vu_name(const char *s) {
  if (strstr(s, "..") || strchr(s, ':') || strchr(s, '\\') || *s == '/')
    return false;
  if (!strcmp(s, "verdant/Image"))
    return true;
  if (!strncmp(s, "verdant/guest/", 14)) {
    const char *n = s + 14;
    if (!*n || strchr(n, '/'))
      return false;
    for (const char *p = n; *p; p++)
      if (!isalnum((unsigned char)*p) && *p != '_' && *p != '-' && *p != '.')
        return false;
    const char *ext = strrchr(n, '.');
    return ext && (!strcmp(ext, ".py") || !strcmp(ext, ".txt") ||
                   !strcmp(ext, ".pem"));
  }
  if (!strcmp(PLAT_SLUG, "3ds"))
    return !strcmp(s, "3ds/verdant/verdant.3dsx") ||
           !strcmp(s, "3ds/verdant/verdant.smdh") ||
           !strcmp(s, "cias/verdant.cia");
  if (!strcmp(PLAT_SLUG, "vita"))
    return !strcmp(s, "verdant.vpk") || !strcmp(s, "app/VRDT00001/eboot.bin") ||
           !strcmp(s, "app/VRDT00001/sce_sys/param.sfo") ||
           !strcmp(s, "app/VRDT00001/sce_sys/icon0.png") ||
           !strcmp(s, "app/VRDT00001/sce_sys/livearea/contents/bg.png") ||
           !strcmp(s, "app/VRDT00001/sce_sys/livearea/contents/startup.png") ||
           !strcmp(s, "app/VRDT00001/sce_sys/livearea/contents/template.xml");
  return false;
}
/* Runs before Linux boots. Each rename is recoverable: staging -> backup ->
 * target. A matching target allows recovery after power loss between rename and
 * plan commit. */
static bool vu_apply(char *message, size_t length) {
  FILE *f = fopen(PLAT_SD "verdant/update-pending/plan.txt", "rb");
  if (!f)
    return true;
  char platform[32], version[48], line[768];
  if (!fgets(platform, sizeof(platform), f) ||
      !fgets(version, sizeof(version), f))
    goto invalid;
  platform[strcspn(platform, "\r\n")] = 0;
  version[strcspn(version, "\r\n")] = 0;
  if (strcmp(platform, PLAT_SLUG))
    goto invalid;
  unsigned a, b, c;
  int tail = 0;
  if (sscanf(version, "%u.%u.%u%n", &a, &b, &c, &tail) != 3 || version[tail])
    goto invalid;
  int count = 0;
  bool has_image = false, has_app = false;
  /* Validate every staged hash before touching a target. */
  while (fgets(line, sizeof(line), f)) {
    line[strcspn(line, "\r\n")] = 0;
    if (strlen(line) < 66 || line[64] != ' ' || !vu_name(line + 65) ||
        ++count > 64)
      goto invalid;
    for (int i = 0; i < 64; i++)
      if (!isxdigit((unsigned char)line[i]) || isupper((unsigned char)line[i]))
        goto invalid;
    line[64] = 0;
    char staged[900], dest[900];
    snprintf(staged, sizeof(staged), PLAT_SD "verdant/update-pending/%s",
             line + 65);
    snprintf(dest, sizeof(dest), PLAT_SD "%s", line + 65);
    if (!vu_hash(vu_exists(staged) ? staged : dest, line))
      goto invalid;
    if (!strcmp(line + 65, "verdant/Image"))
      has_image = true;
    if (!strcmp(line + 65, !strcmp(PLAT_SLUG, "vita")
                               ? "app/VRDT00001/eboot.bin"
                               : "3ds/verdant/verdant.3dsx"))
      has_app = true;
  }
  if (ferror(f) || !has_image || !has_app)
    goto invalid;
  rewind(f);
  if (!fgets(platform, sizeof(platform), f) ||
      !fgets(platform, sizeof(platform), f))
    goto invalid;
  while (fgets(line, sizeof(line), f)) {
    line[strcspn(line, "\r\n")] = 0;
    line[64] = 0;
    char staged[900], dest[900], backup[900];
    snprintf(staged, sizeof(staged), PLAT_SD "verdant/update-pending/%s",
             line + 65);
    snprintf(dest, sizeof(dest), PLAT_SD "%s", line + 65);
    snprintf(backup, sizeof(backup), PLAT_SD "verdant/update-backup/%s/%s",
             version, line + 65);
    if (!vu_exists(staged))
      continue;
    if (!vu_parents(dest) || !vu_parents(backup))
      goto interrupted;
    if (vu_exists(dest) && !vu_exists(backup) && rename(dest, backup))
      goto interrupted;
    if (rename(staged, dest))
      goto interrupted;
  }
  fclose(f);
  if (!strcmp(PLAT_SLUG, "3ds")) {
    int result = plat_update_title(PLAT_SD "cias/verdant.cia");
    if (result < 0) {
      snprintf(message, length,
               "CIA install failed (%d). Retry on relaunch or install "
               "cias/verdant.cia in FBI.",
               result);
      return false;
    }
  }
  FILE *v = fopen(PLAT_SD "verdant/installed-version.part", "wb");
  if (!v) {
    snprintf(message, length,
             "Could not commit update version. Relaunch to retry.");
    return false;
  }
  bool ok = fprintf(v, "%s\n", version) > 0 && !ferror(v);
  if (fclose(v))
    ok = false;
  if (!ok || rename(PLAT_SD "verdant/installed-version.part",
                    PLAT_SD "verdant/installed-version.txt")) {
    snprintf(message, length,
             "Could not commit update version. Relaunch to retry.");
    return false;
  }
  remove(PLAT_SD "verdant/update-pending/plan.txt");
  /* Consumed subdirectories may remain; rename preserves them instead of
   * deleting. */
  char done[256];
  snprintf(done, sizeof(done), PLAT_SD "verdant/update-applied-%s", version);
  if (rename(PLAT_SD "verdant/update-pending", done)) {
    snprintf(message, length,
             "Update applied; consumed staging directory remains.");
    return true;
  }
  snprintf(
      message, length,
      "Update %s applied. Relaunch once more to run the updated executable.",
      version);
  return true;
invalid:
  fclose(f);
  snprintf(message, length,
           "Update rejected: wrong platform, invalid plan or damaged files. "
           "Hold B for recovery.");
  return false;
interrupted:
  fclose(f);
  snprintf(message, length,
           "Update interrupted or storage permission denied. Relaunch to "
           "retry; backups are preserved.");
  return false;
}
#endif
