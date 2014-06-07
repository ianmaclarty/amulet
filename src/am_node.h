struct am_node {
    am_command **commands;
    int recursion_limit;

    void render(am_render_state *rstate);
};
