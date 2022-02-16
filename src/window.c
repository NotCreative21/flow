void node_insert_at_head(node *n) {
    if (head) {
        head->prev = n;
        n->next = head;
    }
    head = n;
}

void node_insert_at_tail(node *n) {
    if (head) {
        node *tmp = head;
        while (tmp->next) tmp = tmp->next;
        tmp->next = n;
        n->prev = tmp;
    } else {
        head = n;
    }
}

void node_remove(node *n) {
    if (n == head) head = n->next;
    if (n->next) n->next->prev = n->prev;
    if (n->prev) n->prev->next = n->next;
}

void node_swap(node *a, node *b) {
    xcb_window_t tmp;
    tmp = a->window;
    a->window = b->window;
    b->window = tmp;
}

void create_window(xcb_window_t window) {
	node *w = (node*)calloc(1, sizeof(node));
	w->window = window;
	w->next = NULL;
	w->prev = NULL;

	//node_insert_at_tail(w);
	current = w;
}

void destroy_window(xcb_window_t window) {
	node *w;

	int8_t i;
	for (i = TAGS; i; --i) {
		w = tagset[i - 1]->head;
		while (w) {
			if (w->window == window)
				goto rest;
			w = w->next;
		}
	}
	if (!w) return;

rest:
	node_remove(w);

	if (w == current) {
		if (w->next)
			current = w->next;
		else
			current = w->prev;
	}

	if (w == tagset[i - 1]->head)
		tagset[i - 1]->head = NULL;
	if (w == tagset[i -1]->current)
		tagset[i - 1]->current = NULL;

	free(w);
}


