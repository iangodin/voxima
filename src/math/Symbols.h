#pragma once

// Math symbols by name, so a formula reads as it would in TeX and its source has no hard-to-type characters in it. Each is the
// symbol as UTF-8 text, to give to the text builders and the atoms:
//
//     using namespace math;
//     row( { ord( text( sym::alpha ) ), rel( sym::leq ), ord( text( sym::beta ) ), bin( sym::pm ), ord( "1" ) } )
//
// Lowercase Greek letters are set in math italic by text(), as TeX does; capital Greek letters are upright in TeX, so set them with
// upright(): upright( sym::Gamma ). Operators, relations and arrows are used with bin(), rel() and ord(), which give them their
// spacing.
namespace math::sym
{

// ---- Greek, lowercase ----------------------------------------------------------------------
inline constexpr const char *alpha = "α";
inline constexpr const char *beta = "β";
inline constexpr const char *gamma = "γ";
inline constexpr const char *delta = "δ";
inline constexpr const char *epsilon = "ϵ";
inline constexpr const char *varepsilon = "ε";
inline constexpr const char *zeta = "ζ";
inline constexpr const char *eta = "η";
inline constexpr const char *theta = "θ";
inline constexpr const char *vartheta = "ϑ";
inline constexpr const char *iota = "ι";
inline constexpr const char *kappa = "κ";
inline constexpr const char *lambda = "λ";
inline constexpr const char *mu = "μ";
inline constexpr const char *nu = "ν";
inline constexpr const char *xi = "ξ";
inline constexpr const char *pi = "π";
inline constexpr const char *varpi = "ϖ";
inline constexpr const char *rho = "ρ";
inline constexpr const char *varrho = "ϱ";
inline constexpr const char *sigma = "σ";
inline constexpr const char *varsigma = "ς";
inline constexpr const char *tau = "τ";
inline constexpr const char *upsilon = "υ";
inline constexpr const char *phi = "ϕ";
inline constexpr const char *varphi = "φ";
inline constexpr const char *chi = "χ";
inline constexpr const char *psi = "ψ";
inline constexpr const char *omega = "ω";

// ---- Greek, capitals (the ones that differ from the Latin ones) ------------------------------
inline constexpr const char *Gamma = "Γ";
inline constexpr const char *Delta = "Δ";
inline constexpr const char *Theta = "Θ";
inline constexpr const char *Lambda = "Λ";
inline constexpr const char *Xi = "Ξ";
inline constexpr const char *Pi = "Π";
inline constexpr const char *Sigma = "Σ";
inline constexpr const char *Upsilon = "Υ";
inline constexpr const char *Phi = "Φ";
inline constexpr const char *Psi = "Ψ";
inline constexpr const char *Omega = "Ω";

// ---- Relations ---------------------------------------------------------------------------------
inline constexpr const char *leq = "≤";
inline constexpr const char *geq = "≥";
inline constexpr const char *neq = "≠";
inline constexpr const char *ll = "≪";
inline constexpr const char *gg = "≫";
inline constexpr const char *approx = "≈";
inline constexpr const char *equiv = "≡";
inline constexpr const char *sim = "∼";
inline constexpr const char *simeq = "≃";
inline constexpr const char *cong = "≅";
inline constexpr const char *propto = "∝";
inline constexpr const char *subset = "⊂";
inline constexpr const char *supset = "⊃";
inline constexpr const char *subseteq = "⊆";
inline constexpr const char *supseteq = "⊇";
inline constexpr const char *in = "∈";
inline constexpr const char *notin = "∉";
inline constexpr const char *ni = "∋";
inline constexpr const char *perp = "⊥";
inline constexpr const char *parallel = "∥";
inline constexpr const char *mid = "∣";

// ---- Binary operators ----------------------------------------------------------------------------
inline constexpr const char *pm = "±";
inline constexpr const char *mp = "∓";
inline constexpr const char *times = "×";
inline constexpr const char *div = "÷";
inline constexpr const char *cdot = "⋅";
inline constexpr const char *ast = "∗";
inline constexpr const char *star = "⋆";
inline constexpr const char *circ = "∘";
inline constexpr const char *oplus = "⊕";
inline constexpr const char *otimes = "⊗";
inline constexpr const char *cap = "∩";
inline constexpr const char *cup = "∪";
inline constexpr const char *wedge = "∧";
inline constexpr const char *vee = "∨";
inline constexpr const char *setminus = "∖";

// ---- Arrows ----------------------------------------------------------------------------------------
inline constexpr const char *to = "→";
inline constexpr const char *leftarrow = "←";
inline constexpr const char *leftrightarrow = "↔";
inline constexpr const char *Rightarrow = "⇒";
inline constexpr const char *Leftarrow = "⇐";
inline constexpr const char *Leftrightarrow = "⇔";
inline constexpr const char *mapsto = "↦";
inline constexpr const char *uparrow = "↑";
inline constexpr const char *downarrow = "↓";

// ---- Other symbols --------------------------------------------------------------------------------------
inline constexpr const char *infty = "∞";
inline constexpr const char *partial = "∂";
inline constexpr const char *nabla = "∇";
inline constexpr const char *forall = "∀";
inline constexpr const char *exists = "∃";
inline constexpr const char *emptyset = "∅";
inline constexpr const char *neg = "¬";
inline constexpr const char *angle = "∠";
inline constexpr const char *degree = "°";
inline constexpr const char *ell = "ℓ";
inline constexpr const char *hbar = "ℏ";
inline constexpr const char *therefore = "∴";
inline constexpr const char *because = "∵";
inline constexpr const char *prime = "′";

}
