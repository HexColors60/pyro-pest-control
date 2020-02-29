#ifndef TEXT_H
#define TEXT_H

void text_init();

void text_log_add(const char *str);

void text_log_render();

void text_render(const char *str, int x, int y);

#endif // TEXT_H