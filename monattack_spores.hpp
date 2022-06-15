#ifndef MONATTACK_SPORES_HPP
#define MONATTACK_SPORES_HPP 1

#ifndef _GAME_H_
#error need game.h included before this
#endif

struct coat_in_spores {
    monster* killer;

    coat_in_spores() noexcept : killer(nullptr) {};
    coat_in_spores(monster* killer) noexcept : killer(killer) {};

    coat_in_spores(const coat_in_spores& src) = delete;
    coat_in_spores(coat_in_spores&& src) = delete;
    coat_in_spores& operator=(const coat_in_spores& src) = delete;
    coat_in_spores& operator=(coat_in_spores&& src) = delete;
    ~coat_in_spores() = default;

    void operator()(monster* target) {
        // \todo? Is it correct for spores to affect digging monsters (current behavior)
        static auto msg = [&]() {
            return grammar::capitalize(target->desc(grammar::noun::role::subject, grammar::article::definite)) + " is covered in tiny spores!";
        };

        target->if_visible_message(msg);
        if (!target->make_fungus()) game::active()->kill_mon(*target, killer);
    }

    void operator()(pc* target) {
        static auto msg = [&]() {
            return grammar::capitalize(target->desc(grammar::noun::role::subject, grammar::article::definite)) + " is covered in tiny spores!";
        };
        target->infect(DI_SPORES, bp_mouth, 4, 30); // Spores hit the player
    }

    // both DI_SPORES and DI_FUNGUS have to become NPC-competent before allowing this
    void operator()(npc* target) {
#if 0
        static auto msg = [&]() {
            return grammar::capitalize(target->desc(grammar::noun::role::subject, grammar::article::definite)) + " is covered in tiny spores!";
        };
        target->infect(DI_SPORES, bp_mouth, 4, 30); // Spores hit the player
#endif
    } // no-op
};

/// returns C-true if a monster was explicitly spored.  Return type matches std::get due to Waterfall/SSADM software lifecycle.
template<class T>
monster* spray_spores(const T& dest, monster* killer) requires requires (game* g) { g->mob_at(dest); }
{
    const auto g = game::active();
    if (auto mob = g->mob_at(dest)) {
        std::visit(coat_in_spores(killer), *mob);
//      return *std::get_if<monster*>(&(*mob)); // fixes GCC 12 warning about returning dangling reference, *BUT* dereferences nullptr!
        if (auto mon = std::get_if<monster*>(&(*mob))) return *mon;
        return nullptr;
    } else {
        g->z.push_back(monster(mtype::types[mon_spore], dest));
        return nullptr;
    }
}

#endif
