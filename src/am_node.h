struct am_node {
    am_command_list *command_list;
    int recursion_limit;

    void render(am_render_state *rstate);
};
